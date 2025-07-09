#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

void
find(char *path, char *target)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  // 打开路径
  if((fd = open(path, O_RDONLY)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  // 获取路径状态
  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  // 如果是普通文件，直接判断名字是否匹配
  if(st.type == T_FILE){
    // 提取最后一段文件名
    char *name;
    for(name = path + strlen(path); name >= path && *name != '/'; name--);
    name++;
    if(strcmp(name, target) == 0){
      printf("%s\n", path);
    }
  }
  // 如果是目录，遍历子项
  else if(st.type == T_DIR){
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
      fprintf(2, "find: path too long\n");
      close(fd);
      return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    // 读取目录项
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      // 不递归 . 和 ..
      if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;
      // 拼接完整路径
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      // 获取子项状态
      if(stat(buf, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", buf);
        continue;
      }
      // 递归调用 find
      find(buf, target);
    }
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc != 3){
    fprintf(2, "usage: find <path> <target>\n");
    exit(1);
  }

  find(argv[1], argv[2]);
  exit(0);
}