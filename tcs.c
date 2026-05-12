//Powered by antichat
//telnet chat server
//19.09.2023
//gcc tcs.c -o tcs
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
 
#define FDS_ARRAY_CHUNK_SIZE 100
#define MAX_MESSAGE_LEN 1728
struct pollfd *fds;
int fds_len;
 
void extend_fds()//extending fds array
{
    int i;
 
    fds = realloc(fds, (fds_len + FDS_ARRAY_CHUNK_SIZE) * sizeof(struct pollfd));
   
    for (i = 0; i < FDS_ARRAY_CHUNK_SIZE; i++) {
        fds[fds_len + i].fd = -1;
        fds[fds_len + i].events = 0;
    }
 
    fds_len += FDS_ARRAY_CHUNK_SIZE;
   
}
 
void fds_set(int fd)//add fd to fds
{
    int i;
 
    for (i = 0; ; i++) {
       
        if (i == fds_len) extend_fds();
 
        if (fds[i].fd < 0) {
            fds[i].fd = fd;
            fds[i].events = POLLIN;
            fds[i].revents = 0;
            break;
        }
    }
}
 
void fds_clr(int fd) // remove fd from fds
{
    int i;
 
    for (i = 0; i < fds_len; i++) {
        if (fds[i].fd == fd) {
            fds[i].fd = -1;
                fds[i].events = 0; 
            break;
        }
    }
}
void set_nonblock(int socket)
{    
    int flags;    
    flags = fcntl(socket,F_GETFL,0);    
    assert(flags != -1);    
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}
void send_to_all(int servfd, int fd,  char *buff, int len)
{
    int i;
 
    for (i = 0; i < fds_len; i++) {
        if (fds[i].fd > 0 && fd != fds[i].fd && fds[i].fd != servfd) {
            write(fds[i].fd, buff, len);//TODO check return value and mark fd as closed
           
        }
    }
}
 
int telnet_negotiate_linemode(int fd)
{
    char do_linemode[] = {255, 253, 34};
    char on_linemode[] = {255, 250, 34, 1, 1, 255, 240};
    char will_echo[] = {255, 253, 1};
    char wont_echo[] = {255, 252, 1};
    char reply_buff[512];
    int n;
 
   
 
    write(fd, do_linemode, sizeof(do_linemode));
    write(fd, on_linemode, sizeof(on_linemode));
    write(fd, will_echo, sizeof(will_echo));
    write(fd, wont_echo, sizeof(wont_echo));
 
    read(fd, reply_buff, sizeof(reply_buff));
 
    return 0;
 
}
 
void replace_commands_with_spaces(char *buff, int len)
{
    int i;
    unsigned char c;
 
    for (i = 0; i < len && buff[i] != 0; i++) {
        c = buff[i];
        if (c < ' ' || c > 126) {
            c = ' ';//replacing with spaces
        }  
    }
}
int main(int argc, char** argv)
{
    struct sockaddr_in servaddr;
    int servfd, connfd, port, pollret;
    char *host, buff[MAX_MESSAGE_LEN], cleaned_buff[MAX_MESSAGE_LEN];
    int i, msg_len, on, negresult;
   
    fds = NULL;
    fds_len = 0;
    on  = 1;
 
    setbuf(stdout, NULL);  
   
    if (argc != 3) {
        perror("Please provide host and port. Example: telnet-chat 0.0.0.0 4022\r\n");
        exit(1);
    }  
 
    memset(&servaddr, 0, sizeof(servaddr));
    servfd = socket(AF_INET, SOCK_STREAM, 0);
 
    host = argv[1];
    port = atoi(argv[2]);
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(host);
    servaddr.sin_port = htons(port);
 
    bind(servfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
 
    listen(servfd, 1024);
 
    ioctl(servfd, FIONBIO, (char*)&on);
   
    fds_set(servfd);
 
    while(1) {
        pollret = poll(fds, fds_len, 100);
 
        for (i = 0; i < fds_len; i++) {
            if (fds[i].fd > 0) {
                if (fds[i].revents & POLLIN) {
                    if (fds[i].fd == servfd) {
                        connfd = accept(servfd, (struct sockaddr*)NULL, NULL);
                        telnet_negotiate_linemode(connfd);
                        fds_set(connfd);
                    } else {
                        msg_len = read(fds[i].fd, buff, MAX_MESSAGE_LEN);
                        replace_commands_with_spaces(buff, msg_len);
                        send_to_all(servfd, fds[i].fd, buff, msg_len);
                    }
                }
                   
                if(fds[i].revents & POLLHUP ) {            
                    //closed by peer            
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    fds[i].events = 0;
                }
            }
        }
       
    }
    return 0;
 
}