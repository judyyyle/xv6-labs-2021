#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p2c[2]; // parent to child
    int c2p[2]; // child to parent
    pipe(p2c);
    pipe(c2p);

    if (fork() == 0) {
        // child
        char buf[1];
        read(p2c[0], buf, 1);
        close(p2c[0]);
        fprintf(0, "%d: received ping\n", getpid());
        write(c2p[1], buf, 1);
        close(c2p[1]);
        exit(0);
    } else {
        // parent
        char buf[1] = {'a'};
        write(p2c[1], buf, 1);
        close(p2c[1]);
        read(c2p[0], buf, 1);
        fprintf(0, "%d: received pong\n", getpid());
        close(c2p[0]);
        wait(0);
        exit(0);
    }
}