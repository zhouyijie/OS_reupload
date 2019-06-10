#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
int main()
{
    printf("initiate redirect\n");
    
    //output redir
    int fd_1 = open("redirect.txt",  O_WRONLY | O_CREAT, 0666);
                
    close(1);
    int dup_fd = dup(fd_1);
    printf("A simple program output.");
    close(fd_1);
    return 0;
}
