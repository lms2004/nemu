#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>

jmp_buf env;

void handler(int signal) {
    longjmp(env, 2);
}

int main(int argc, char *argv[]) {
    {
        struct sigaction sa = {};
        sa.sa_handler = handler;
        if (sigaction(SIGFPE, &sa, NULL) == -1) {
            perror("sigaction");
        }
    }

    if (0 == setjmp(env)) {
        unsigned result = 1 / 0;  // Example division by zero to trigger SIGFPE
        printf("%u", result);  // This will not be printed
    } else {
        printf("%u", 65535);  // This will be printed after handling the exception
    }

    return 1;
}
