#include "http_conn.h"
#include <cerrno>
#include <cmath>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/stat.h>

// 定义HTTP响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form =
    "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form =
    "You do not have permission to get file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form =
    "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form =
    "There was an unusual problem serving the requested file.\n";

int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;
// 网站的根目录
const char *doc_root = "/workspaces/Web-Server/webServer/resources";

// 设置文件描述符非阻塞
void setnonblocking(int fd) {
  int old_flag = fcntl(fd, F_GETFL);
  int new_flag = old_flag | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_flag);
}

// 向epoll中添加文件描述符
void addfd(int epollfd, int fd, bool one_shot) {
  epoll_event event;
  event.data.fd = fd;
  // 监听读事件和断开连接事件
  event.events = EPOLLIN | EPOLLRDHUP; // 加入 EPOLLET边沿触发

  if (one_shot) {
    // 触发一次后就不再监听该fd，必须要重新注册
    // 保证一个线程在处理某个socket连接时，其他线程不处理该连接
    event.events |= EPOLLONESHOT;
  }
  // 向 epollfd 这个监控中心，添加对 fd 的监控，监控的规则定义在 event 结构体里
  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
  // 设置文件描述符为非阻塞
  setnonblocking(fd);
}

// 从epoll中移除监听的文件描述符
void removefd(int epollfd, int fd) {
  epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
  close(fd);
}

// 修改文件描述符，重置socket上的EPOLLONESHOT事件，以确保下一次可读时，EPOLLIN事件能被触发
void modfd(int epollfd, int fd, int ev) {
  epoll_event event;
  event.data.fd = fd;

  event.events = ev | EPOLLONESHOT | EPOLLRDHUP; // EPOLLET边沿触发
  epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// 初始化连接
void http_conn::init(int sockfd, const sockaddr_in &addr) {
  m_sockfd = sockfd;
  m_address = addr;

  // 设置端口复用
  int reuse = 1;
  setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  // 添加到epoll对象
  addfd(m_epollfd, sockfd, true);
  m_user_count++;

  init(); // 用于初始化连接其余的信息
}
// 初始化连接其余的信息
void http_conn::init() {
  m_check_state = CHECK_STATE_REQUESTLINE; // 初始化状态为分析请求首行
  m_checked_idx = 0;
  m_start_line = 0;
  m_read_idx = 0;

  m_url = nullptr;     // 请求目标文件的文件名
  m_version = nullptr; // 协议版本，目前支持HTTP1.1
  m_method = GET;      // 请求方法
  m_linger = false;    // http请求是否保持连接

  bzero(m_read_buf, READ_SIZE);
}
// 关闭连接
void http_conn::close_conn() {
  if (m_sockfd != -1) {            // 防止重复关闭
    removefd(m_epollfd, m_sockfd); // 将文件描述符从epoll中删除，并关闭
    m_sockfd = -1;
    m_user_count--;
  }
}

bool http_conn::read() {
  if (m_read_idx >= READ_SIZE) {
    return false;
  }

  int bytes_read = 0;
  while (true) {
    bytes_read =
        recv(m_sockfd, m_read_buf + m_read_idx, READ_SIZE - m_read_idx, 0);
    if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // 没有数据了
        break;
      }
      return false;
    } else if (bytes_read == 0) {
      // 对方关闭连接
      return false;
    }
    m_read_idx += bytes_read;
  }
  printf("read data:%s\n", m_read_buf);
  return true;
}
// 主状态机，解析请求
http_conn::HTTP_CODE http_conn::process_read() {
  LINE_STATUS line_status = LINE_OK;
  HTTP_CODE ret = NO_REQUEST;
  char *text = 0;
  while ((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK) ||
         (line_status = parse_line()) == LINE_OK) {
    // 当进入请求体时，由于请求体没有\r\n结尾，所以parse_line()会返回LINE_OPEN
    // TODO B部分是否有必要存在
    text = get_line();
    m_start_line = m_checked_idx;
    printf("got 1 http line:%s\n", text);

    switch (m_check_state) {
    case CHECK_STATE_REQUESTLINE: {
      ret = parse_request_line(text);
      if (ret == BAD_REQUEST) {
        return BAD_REQUEST;
      }
      break; // 仅跳出switch，必备
    }
    case CHECK_STATE_HEADER: {
      ret = parse_headers(text);
      if (ret == BAD_REQUEST) {
        return BAD_REQUEST;
      } else if (ret == GET_REQUEST) { // 处理GET请求
        return do_request();
      }
      break;
    }
    case CHECK_STATE_CONTENT: {
      ret = parse_content(text);
      if (ret == GET_REQUEST) {
        return do_request();
      }
      line_status = LINE_OPEN; // 代表行数据尚不完整
      break;
    }
    default: {
      return INTERNAL_ERROR;
    }
    }
    // return NO_REQUEST;
  }
  return NO_REQUEST;
} // 解析HTTP请求

// 解析请求首行，获取请求方法、目标URL，HTTP版本
http_conn::HTTP_CODE http_conn::parse_request_line(char *text) {
  // GET /index.html HTTP/1.1
  // 在一个字符串中查找另一个字符串中任意一个字符首次出现的位置，并返回一个指向该位置的指针
  m_url = strpbrk(text, " \t"); // 找到第一个空格或\t的位置
  if (!m_url) {
    return BAD_REQUEST;
  }
  *m_url++ = '\0'; // 将空格或\t替换为\0，并将m_url指向下一个字符

  char *method = text; // method指向请求方法
  // 忽略大小写比较两个字符串是否相等。
  if (strcasecmp(method, "GET") == 0) {
    m_method = GET;
  } else {
    return BAD_REQUEST;
  }
  m_version = strpbrk(m_url, " \t");
  if (!m_version) {
    return BAD_REQUEST;
  }
  *m_version++ = '\0';
  if (strcasecmp(m_version, "HTTP/1.1") != 0) {
    return BAD_REQUEST;
  }
  // 忽略大小写比较两个字符串的前 n 个字符是否相等。
  if (strncasecmp(m_url, "http://", 7) == 0) {
    m_url += 7;
    // 在一个字符串中查找某一个特定字符首次出现的位置，并返回一个指向该位置的指针
    m_url = strchr(m_url, '/');
  }
  // “如果 m_url 是一个空指针（意味着在绝对 URL 中没找到路径部分），或者 m_url所
  // 指向的路径不是以斜杠开头，那么这个请求就是无效的”
  if (!m_url || m_url[0] != '/') {
    return BAD_REQUEST;
  }
  m_check_state = CHECK_STATE_HEADER; // 主状态机转移到检查请求头状态

  return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_headers(char *text) {
  // 遇到空行，表示头部字段解析完毕
  if (text[0] == '\0') {
    // 如果HTTP请求有消息体，则还需要读取m_content_length字节的消息体，
    // 状态机转移到CHECK_STATE_CONTENT状态
    if (m_content_length != 0) {
      m_check_state = CHECK_STATE_CONTENT;
      return NO_REQUEST;
    }
    // 否则说明我们已经得到了一个完整的HTTP请求
    return GET_REQUEST;
  } else if (strncasecmp(text, "Connection:", 11) == 0) {
    // 处理Connection 头部字段  Connection: keep-alive
    text += 11;
    text += strspn(text, " \t");
    if (strcasecmp(text, "keep-alive") == 0) {
      m_linger = true;
    }
  } else if (strncasecmp(text, "Content-Length:", 15) == 0) {
    // 处理Content-Length头部字段（只有在有消息体时才会出现，GET请求不会出现）
    text += 15;
    text += strspn(text, " \t");
    m_content_length = atol(text);
  } else if (strncasecmp(text, "Host:", 5) == 0) {
    // 处理Host头部字段
    text += 5;
    text += strspn(text, " \t");
    m_host = text;
  } else {
    printf("oop! unknow header %s\n", text);
  }
  return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_content(char *text) {
  // >=右侧计算出请求体结束位置的下一个字节在 m_read_buf 中应该在的索引
  // 代表意义是已经接收到的总字节数（m_read_idx），是否已经足够覆盖到整个请求体
  if (m_read_idx >= (m_content_length + m_checked_idx)) {
    text[m_content_length] = '\0';
    return GET_REQUEST;
  }
  return NO_REQUEST;
}

http_conn::LINE_STATUS http_conn::parse_line() {
  char temp;
  for (; m_checked_idx < m_read_idx; ++m_checked_idx) {
    temp = m_read_buf[m_checked_idx];
    if (temp == '\r') {
      if ((m_checked_idx + 1) == m_read_idx) {
        return LINE_OPEN; // 我们无法确定下一个字节是不是\n，因为还没收到。所以数据行不完整
      } else if (m_read_buf[m_checked_idx + 1] == '\n') {
        m_read_buf[m_checked_idx++] =
            '\0'; // 将解析出的每一行当作一个标准的 C 语言字符串来处理
        m_read_buf[m_checked_idx++] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    } else if (temp == '\n') {
      // 这部分代码是无效的
      if ((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == '\r')) {
        m_read_buf[m_checked_idx - 1] = '\0';
        m_read_buf[m_checked_idx++] = '\0';
        return LINE_OK;
      }
      // 如果'\n'前不是'\r'，也是错误的
      return LINE_BAD;
    }
  }
  // 如果for循环正常结束，说明扫描完了所有数据都没找到完整的行
  return LINE_OPEN;
} // 解析一行，判断依据\r\n

http_conn::HTTP_CODE http_conn::do_request() {
  // "/home/nowcoder/webserver/resources"
  strcpy(m_real_file, doc_root); // 将网站根目录复制到m_real_file
  int len = strlen(doc_root);
  // 将网站的根目录和客户端请求的 URL
  // 路径拼接起来，最后一个参数是允许复制的最大长度
  strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
  // 获取m_real_file文件的相关的状态信息，-1失败，0成功
  if (stat(m_real_file, &m_file_stat) < 0) {
    return NO_RESOURCE;
  }
  // 判断访问权限
  if (!(m_file_stat.st_mode & S_IROTH)) {
    return FORBIDDEN_REQUEST;
  }
  // 判断是否是目录
  if (S_ISDIR(m_file_stat.st_mode)) {
    return BAD_REQUEST;
  }
  // 以只读方式打开文件
  int fd = open(m_real_file, O_RDONLY);
  // 创建内存映射
  m_file_address =
      (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  return FILE_REQUEST;
}

// 对内存映射区执行munmap操作，释放资源
void http_conn::unmap() {
  if (m_file_address) {
    munmap(m_file_address, m_file_stat.st_size);
    m_file_address = 0;
  }
}

// 写HTTP响应
bool http_conn::write() {
  int temp = 0;
  int bytes_have_send = 0; // 已经发送的字节
  int bytes_to_send =
      m_write_idx; // 将要发送的字节 （m_write_idx）写缓冲区中待发送的字节数

  if (bytes_to_send == 0) {
    // 将要发送的字节为0，这一次响应结束。
    modfd(m_epollfd, m_sockfd, EPOLLIN);
    init();
    return true;
  }

  while (1) {
    // 分散写
    temp = writev(m_sockfd, m_iv, m_iv_count);
    if (temp <= -1) {
      // 如果TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件，虽然在此期间，
      // 服务器无法立即接收到同一客户的下一个请求，但可以保证连接的完整性。
      if (errno == EAGAIN) {
        modfd(m_epollfd, m_sockfd, EPOLLOUT);
        return true;
      }
      unmap();
      return false;
    }
    bytes_to_send -= temp;
    bytes_have_send += temp;
    if (bytes_to_send <= bytes_have_send) {
      // 发送HTTP响应成功，根据HTTP请求中的Connection字段决定是否立即关闭连接
      unmap();
      if (m_linger) {
        init();
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return true;
      } else {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return false;
      }
    }
  }
}

// 往写缓冲中写入待发送的数据
bool http_conn::add_response(const char *format, ...) {
  if (m_write_idx >= WRITE_SIZE) {
    return false;
  }
  va_list arg_list;
  va_start(arg_list, format);
  int len = vsnprintf(m_write_buf + m_write_idx, WRITE_SIZE - 1 - m_write_idx,
                      format, arg_list);
  if (len >= (WRITE_SIZE - 1 - m_write_idx)) {
    return false;
  }
  m_write_idx += len;
  va_end(arg_list);
  return true;
}

bool http_conn::add_status_line(int status, const char *title) {
  return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_headers(int content_len) {
  return add_content_length(content_len) && add_content_type() &&
         add_linger() && add_blank_line();
}

bool http_conn::add_content_length(int content_len) {
  return add_response("Content-Length: %d\r\n", content_len);
}

bool http_conn::add_linger() {
  return add_response("Connection: %s\r\n",
                      (m_linger == true) ? "keep-alive" : "close");
}

bool http_conn::add_blank_line() { return add_response("%s", "\r\n"); }

bool http_conn::add_content(const char *content) {
  return add_response("%s", content);
}

bool http_conn::add_content_type() {
  return add_response("Content-Type:%s\r\n", "text/html");
}

bool http_conn::process_write(HTTP_CODE ret) {
  switch (ret) {
  case INTERNAL_ERROR:
    add_status_line(500, error_500_title);
    add_headers(strlen(error_500_form));
    if (!add_content(error_500_form)) {
      return false;
    }
    break;
  case BAD_REQUEST:
    add_status_line(400, error_400_title);
    add_headers(strlen(error_400_form));
    if (!add_content(error_400_form)) {
      return false;
    }
    break;
  case NO_RESOURCE:
    add_status_line(404, error_404_title);
    add_headers(strlen(error_404_form));
    if (!add_content(error_404_form)) {
      return false;
    }
    break;
  case FORBIDDEN_REQUEST:
    add_status_line(403, error_403_title);
    add_headers(strlen(error_403_form));
    if (!add_content(error_403_form)) {
      return false;
    }
    break;
  case FILE_REQUEST:
    add_status_line(200, ok_200_title);
    add_headers(m_file_stat.st_size);
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv[1].iov_base = m_file_address;
    m_iv[1].iov_len = m_file_stat.st_size;
    m_iv_count = 2;
    return true;
  default:
    return false;
  }

  m_iv[0].iov_base = m_write_buf;
  m_iv[0].iov_len = m_write_idx;
  m_iv_count = 1;
  return true;
}

// 由线程池中的工作线程调用，处理http请求的入口函数
void http_conn::process() {
  // 解析HTTP请求
  HTTP_CODE read_ret = process_read();
  if (read_ret == NO_REQUEST) {
    modfd(m_epollfd, m_sockfd, EPOLLIN);
    return;
  }

  // 生成响应
  bool write_ret = process_write(read_ret);
  if (!write_ret) {
    close_conn();
  }
  modfd(m_epollfd, m_sockfd, EPOLLOUT);
}