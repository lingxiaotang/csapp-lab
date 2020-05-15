#include<stdio.h>
#include<unistd.h>
#include<signal.h>


void sig_handler(int sig)
{
    printf("hello world\n");
}

int main()
{
    if(signal(SIGINT,sig_handler)==SIG_ERR)
        printf("signal error!\n");

    for(int i=0;i<10;i++)
        sleep(2);
    return 0;
}