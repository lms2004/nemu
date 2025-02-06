// #include <stdio.h>
// #include <signal.h>
// #include <setjmp.h>
// #include <stdlib.h>


// jmp_buf env;

// void handler(int signal) {
//     write(1, "handler\n", 8);
//     sleep(2);
//     longjmp(env, 2);
// }

// int main(int argc, char *argv[]) {
    
//     struct sigaction sa;
//     sa.sa_handler = handler;
//     sigemptyset(&sa.sa_mask);
//     sa.sa_flags = SA_SIGINFO;  // Enable extra signal info


//     for(int i = 0;i < 2;i++){
//             // 使用 sigaction 设置新的信号处理动作
//         if (sigaction(SIGFPE, &sa, NULL) == -1) {
//             perror("sigaction");
//             return 1;
//         }
//         if (0 == setjmp(env)) {
//             unsigned result = 1/0;
//             pause(); 
//             write(1, &result, sizeof(result));
//         } else {
//             write(1, "catched\n", 8);
//         }
//     }

//     return 1;
// }


#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>

jmp_buf fpe;

void handler(int signum)
{
    write(1, "handler\n", 8);
    // Do stuff here then return to execution below
    longjmp(fpe, 1);
}

int main()
{
    int i, j;
    for(i = 0; i < 10; i++) 
    {
        // Call signal handler for SIGFPE
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = handler;
        act.sa_flags = SA_NODEFER;
        sigaction(SIGFPE, &act, NULL);

        if (0 == setjmp(fpe))
        {
            j = i / 0;
        } else {
        }
    }

    printf("After for loop");

    return 0;
}