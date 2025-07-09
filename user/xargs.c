#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
  char buf[512];
  char *p = buf;
  char *args[MAXARG];
  int i, n;

  // 参数数量
  int arg_count = argc - 1;

  // 先放入命令行参数
  for (i = 1; i < argc; i++) {
    args[i - 1] = argv[i];
  }

  while ((n = read(0, p, 1)) > 0) {
    if (*p == '\n') {
      *p = 0;

      args[arg_count] = buf;
      args[arg_count + 1] = 0;

      if (fork() == 0) {
        exec(args[0], args);
        fprintf(2, "xargs: exec failed\n");
        exit(1);
      }
      wait(0);
      p = buf;
    } else {
      p++;
      if (p - buf >= sizeof(buf) - 1) {
        fprintf(2, "xargs: input too long\n");
        exit(1);
      }
    }
  }

  // 处理最后一行（如果没有换行）
  if (p != buf) {
    *p = 0;
    args[arg_count] = buf;
    args[arg_count + 1] = 0;
    if (fork() == 0) {
      exec(args[0], args);
      fprintf(2, "xargs: exec failed\n");
      exit(1);
    }
    wait(0);
  }

  exit(0);
}
