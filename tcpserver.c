#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>



#define BUF_LENGTH (1024)

#define ERROR_NORMAL (-1)

char recvbuf [BUF_LENGTH] = {0};
char sendbuf [BUF_LENGTH] = {0};

//#define NORMAL
#undef NORMAL


#define PSELECT
#undef PSELECT

#define PEPOLL

int main ( int argc, char *argv[])
{

#if defined (NORMAL)
printf("Test server!\n");
	//1. 建立socket 套接字
	int listen_fd = 0;
	listen_fd = socket( AF_INET, SOCK_STREAM, 0);
	if ( listen_fd == ERROR_NORMAL ) {
		perror("socket error!\n");
		return ERROR_NORMAL;
	}

	// 2. server bind
	struct sockaddr_in  serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(9999);

	int retbind = bind(listen_fd, (const struct sockaddr *)&serveraddr, (socklen_t)sizeof(serveraddr));
	if ( retbind == ERROR_NORMAL ) {
		perror("bind error");
	}

	// 3. listen
	int retlisten = listen (listen_fd, 10);
	if ( retlisten == ERROR_NORMAL ) {
		perror("listen error!\n");
	}

	// 4. accept
	struct sockaddr_in clientaddr;
	socklen_t len = sizeof(clientaddr);


#if 1
	int flag = fcntl(listen_fd, F_GETFL, 0);

	fcntl(listen_fd, F_SETFL, flag|O_NONBLOCK);
#endif

	int client_fd = accept(listen_fd, (struct sockaddr *)&clientaddr, (socklen_t *)&len);
	if (client_fd == ERROR_NORMAL) {
		perror("accept error!");
	}

	while(1) {
		char buf[BUF_LENGTH] = {0};

		int recvlen = recv(client_fd, buf, BUF_LENGTH, 0);
		if (recvlen == EAGAIN) {
			printf("NONOBLOCK EAGIN\n");
			continue;
		}else if (recvlen == EWOULDBLOCK) {
			printf("NONOBLOCK EWOULDBLOCK\n");
			continue;
		}else {
			//printf("error :%d  %d\n",recvlen, errno);
			//perror("recv error");
		}

		sleep(2);


		printf("revelen: %d   buf:%s\n",recvlen, buf);
	}
    printf("Test server end!\n");

#endif


/*** listen select*/

#if defined (PSELECT)

    printf("Test select!\n");

	// 1. 创建socket
	int listen_fd = 0;
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if ( listen_fd == ERROR_NORMAL ) {

		perror("socket error:");
		printf("socket error num: %d --%s\n",errno, strerror(errno));

		return ERROR_NORMAL;
	}

	// 2. bind server addr
	struct sockaddr_in serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(9999);

	int retbind = bind ( listen_fd, (struct sockaddr *)&serveraddr, (socklen_t )sizeof(serveraddr));
	if ( retbind == ERROR_NORMAL ) {
		perror("bind error");
		printf("bind error :%d \n",errno);

		return ERROR_NORMAL;
	}

	// 3.listen
	int retlisten = listen ( listen_fd, 10);
	if ( retlisten == ERROR_NORMAL ) {
		perror("listen error");
		printf("error number: %d\n",errno);
		return ERROR_NORMAL;
	}


	// 4. 设置socket 为 O_NONBLOCK

	//int flag  = fcntl (listen_fd, F_GETFL, 0);
	//flag |= O_NONBLOCK;

	//fcntl (listen_fd, F_SETFL, flag);

	fd_set rnfds, wnfds, rset, wset;
	int max_fd;
	int client_fd = 0;
    int nready  = 0;
    int i = 0;
    int nByte = 0;

	FD_ZERO (&rnfds);
	FD_ZERO (&wnfds);

	FD_SET ( listen_fd, &rnfds);

    max_fd = listen_fd;

	while (1) {

        rset = rnfds;
        wset = wnfds;

        nready = select(max_fd +1, &rset, &wset, NULL, NULL);

        if (FD_ISSET(listen_fd, &rset)) {  /** <  listen fd */

            struct sockaddr clientaddr;
            socklen_t len = sizeof(clientaddr);

            client_fd = accept(listen_fd, (struct sockaddr *)&clientaddr, (socklen_t *)&len);
            if ( client_fd == ERROR_NORMAL ) {

                // 错误信息记录到日志文件中
                printf("accept socket error: %s(errno: %d)\n", strerror(errno), errno);
		        return 0;
                // log_record();
            }

            FD_SET(client_fd, &rnfds);

           // max_fd =  (client_fd > max_fd ? client_fd : max_fd);
            if (client_fd > max_fd) max_fd = client_fd;
            if (-- nready ) {
                continue;
            }

        }

        for (i = listen_fd +1; i < max_fd+1 ; i++ ) {

            if (FD_ISSET(i, &rset)) {

                // recv ;
                memset(recvbuf, 0 ,BUF_LENGTH);

                nByte = recv (i, recvbuf, BUF_LENGTH, 0);
                if (nByte > 0) {

                    recvbuf[nByte] = '\0';

                    printf("recvbuf(session:%d):%s\n",i, recvbuf);

                    memset(sendbuf, 0, BUF_LENGTH);
                    strcpy(sendbuf,recvbuf);

                    FD_SET(i, &wnfds); // 句柄加入到写位图

                } else if (nByte == 0) {
                    FD_CLR (i, &rnfds);
                    close(i);
                }

                if ( --nready == 0) {
                    break;
                }
            }else if (FD_ISSET(i, &wset)) {

                //发送数据

                nByte = send(i, recvbuf, strlen(recvbuf), 0);
                if

                FD_SET(i,&rnfds);
            }
        }

	}

#endif

#if defined(PEPOLL)

printf("Test epollt!\n");

 // 1. 创建socket
 int listen_fd = 0;
 listen_fd = socket(AF_INET, SOCK_STREAM, 0);
 if ( listen_fd == ERROR_NORMAL ) {

     perror("socket error:");
     printf("socket error num: %d --%s\n",errno, strerror(errno));

     return ERROR_NORMAL;
 }

 // 2. bind server addr
 struct sockaddr_in serveraddr;

 memset(&serveraddr, 0, sizeof(serveraddr));

 serveraddr.sin_family = AF_INET;
 serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
 serveraddr.sin_port = htons(9999);

 int retbind = bind ( listen_fd, (struct sockaddr *)&serveraddr, (socklen_t )sizeof(serveraddr));
 if ( retbind == ERROR_NORMAL ) {
     perror("bind error");
     printf("bind error :%d \n",errno);

     return ERROR_NORMAL;
 }

 // 3.listen
 int retlisten = listen ( listen_fd, 10);
 if ( retlisten == ERROR_NORMAL ) {
     perror("listen error");
     printf("error number: %d\n",errno);
     return ERROR_NORMAL;
 }


    // 创建epoll

    int epoll_fd = 0;
    epoll_fd = epoll_create(1);
    if (epoll_fd == ERROR_NORMAL) {
        printf("create epoll error(errno:%d):%s",errno,strerror(errno));
        return ERROR_NORMAL;
    }

    int i = 0;
#define EPOLL_EVENT_NUM  1024
    struct epoll_event ev, events[EPOLL_EVENT_NUM];

    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD,listen_fd, (struct epoll_event *)&ev);

    while (1) {
        int nready = epoll_wait(epoll_fd, events, EPOLL_EVENT_NUM, 5);
        if (nready == ERROR_NORMAL ) {
            continue;
        }

        for (i = 0; i < nready; i ++)

            if (events[i].data.fd == listen_fd) {
                struct sockaddr clientaddr;
                socklen_t len = sizeof(clientaddr);

                int conn_fd =  accept(listen_fd, (struct sockaddr *)&clientaddr, (socklen_t *)&len);
                if (conn_fd == ERROR_NORMAL ) {
                    printf("accept error(errno:%d):%s", errno, strerror(errno));
                    continue;
                }

                ev.events = EPOLLIN;
                ev.data.fd = conn_fd;

                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev);

            } else if (events[i].events &EPOLLIN) {
                int nByte = recv(events[i].data.fd, recvbuf,BUF_LENGTH, 0);
                 printf("nByte:%d \n",nByte);
                if (nByte > 0) {
                    recvbuf[nByte] = '\0';
                    printf("recvbuf:%s \n",recvbuf);

                    send(events[i].data.fd, recvbuf, nByte, 0);

                } else if (nByte == 0) {

                    ev.events =  EPOLLIN;
                    ev.data.fd = events[i].data.fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL,events[i].data.fd, (struct epoll_event *)&ev );
                    close(events[i].data.fd);
                }
            }


    }

#endif






}







