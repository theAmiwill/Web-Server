#定义变量
src=$(wildcard  ./*.c)
objs=$(patsubst %.c, %.o, $(src))
target=test1

$(target):$(objs)
	$(CC) $(objs) -o $(target)

%.o:%.c
	$(CC) -c $< -o $@

#生明为一个伪目标
.PHONY:clean
clean:
	rm $(objs) -f


