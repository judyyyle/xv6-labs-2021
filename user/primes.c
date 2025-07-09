#include "kernel/types.h"
#include "user/user.h"
void primes(int p0) __attribute__((noreturn));

void primes(int p0){
    int prime;
    if (read(p0, &prime, sizeof(prime)) == 0) {
        close(p0);
        exit(0);
    }
    printf("prime %d\n", prime);

    if (prime * prime > 280) {
        int n;
        while (read(p0, &n, sizeof(n))) {
            if (n % prime != 0)
                printf("prime %d\n", n);
        }
        close(p0);
        exit(0);
    }
    
    int p[2];
    if (pipe(p) < 0) {    
        fprintf(2, "pipe failed\n");
        exit(0);
    }
    if(fork()==0){
        close(p[1]);
        primes(p[0]);
    }
    else{
        close(p[0]);
        int n;
        while (read(p0, &n, sizeof(n))){
            if (n % prime != 0) {
                write(p[1], &n, sizeof(n));
            }
        }
        close(p0);
        close(p[1]);
        wait(0);
        exit(0);
    }         
}

int
main(int argc, char *argv[])
{
    int p[2];
    if (pipe(p) < 0) {
        fprintf(2, "pipe failed\n");
        exit(1);
    }

    if(fork()==0){
        close(p[1]);
        primes(p[0]);
    }
    else{
        close(p[0]);
        for(int i = 2; i <= 280; i++){
            write(p[1], &i, sizeof(i));
        }
        close(p[1]); 
        wait(0);
        exit(0);
    }
}