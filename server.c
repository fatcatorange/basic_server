#include <stdio.h>     // for fprintf()
#include <unistd.h>    // for close(), read()
#include <sys/epoll.h> // for epoll_create1(), epoll_ctl(), struct epoll_event
#include <string.h> 
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>


# define serverPort 12345
# define MAX_LINK_NUM 5
# define MAX_EVENTS 1024
# define BUFF_LENGTH 320

int main() {
    char buf[320] = {0};

    int socket_fd = socket(PF_INET, SOCK_STREAM ,0);

    int reuse = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("Set socket option failed");
        close(socket_fd);
        return -1;
    }


    if(socket_fd < 0) {
        printf("Fail to create a socket."); 
    }

    struct sockaddr_in serverAddr = {
        .sin_family = AF_INET, //IPV4
        .sin_addr.s_addr = INADDR_ANY, //不限 ip
        .sin_port = htons(serverPort)
    };

    if (bind(socket_fd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind socket failed!");
        close(socket_fd);
        exit(0);
    }

    printf("listening...\n");

    listen(socket_fd, MAX_LINK_NUM);

    struct sockaddr_in clientAddr;
    int len = sizeof(clientAddr);

    int epoll_fd = epoll_create(MAX_EVENTS);

    if(epoll_fd == -1) {
        printf("epoll_create error!\n");
        return -3;
    }

    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];

    ev.events = EPOLLIN;
    ev.data.fd = socket_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD ,socket_fd, &ev) < 0) {
        printf("epoll_ctl error!\n");
        return -4;
    }

    int connfd = 0;
    int count = 0;
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS,-1);
        if (nfds == -1) {
            return -5;
        }

        printf("nfds: %d\n",nfds);
       
        for(int i = 0;i < nfds;i++) {
            if(events[i].data.fd == socket_fd) {
                connfd = accept(socket_fd, (struct sockaddr*)&clientAddr, &len);
                ev.events = EPOLLIN;
                ev.data.fd = connfd;

                if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD, connfd , &ev) == -1) {
                    printf("error");
                    return -1;
                }

                printf("accept client: %s\n", inet_ntoa(clientAddr.sin_addr));
                printf("client %d\n",++count);
            }
            else {
                int confd = events[i].data.fd;
                char buff[BUFF_LENGTH];
                int ret1 = read(confd, buff, BUFF_LENGTH);
                if (ret1 == -1) {
                    perror("Read error");
                } else if (ret1 == 0) {
                    close(confd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, confd, NULL); 
                } else {
                    printf("%d %s\n",confd, buff);
                    int ret2 = write(confd, buff, ret1); 
                    if (ret2 == -1) {
                        perror("Write error");
                    }
                }
            }
            
        }

    }
    if (close(socket_fd) < 0) {
        perror("close socket failed!");
    }
    
    return 0;
}