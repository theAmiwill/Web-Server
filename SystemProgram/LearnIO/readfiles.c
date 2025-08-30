#include<sys/types.h>
#include<dirent.h> 
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int getfilenum(const char* path);

int main(int argc, char* argv[]){
    if(argc<2){
        printf("%s path\n",argv[0]);
        return -1;
    }
    int num=getfilenum(argv[1]);
    printf("文件个数：%d\n",num);
    return 0;
}

int getfilenum(const char* path){
    DIR * dir=opendir(path);
    if(dir==NULL){
        perror("opendir");
        return -1;
    }
    struct dirent* ptr=NULL;

    int total=0;

    while ((ptr=readdir(dir))!=NULL){
        char* dname=ptr->d_name;
        if(strcmp(dname,".")==0 || strcmp(dname,"..")==0){
            continue;
        }
        if(ptr->d_type==DT_DIR){
            char newpath[512];
            snprintf(newpath, sizeof(newpath), "%s/%s", path, dname);
            total+=getfilenum(newpath);
        }
        if(ptr->d_type==DT_REG){
            total++;
        }
    }
    closedir(dir);
    return total;
}