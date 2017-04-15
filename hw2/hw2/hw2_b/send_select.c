#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#define BUFFER 1400

int timeo(int fd)
{
	 	fd_set rset;
        struct timeval tv;

        FD_ZERO(&rset);
        FD_SET(fd, &rset);

        tv.tv_sec = 0;
        tv.tv_usec = 5000;

	return (select(fd +1, &rset, NULL,NULL,&tv));
}
void fsend(int sockfd,struct sockaddr* pservaddr,socklen_t servlen,FILE* fp)
{
	socklen_t len = servlen;
	
    int n;
    char send_buffer[BUFFER];
	char ack[sizeof(long long int)];
	long long int order = -1;
	long long int check = -1;
	int rb;
    while(!feof(fp))
	{
		
		if(order == check)
		{
			order++;
			rb =  fread(send_buffer,1,BUFFER-sizeof(long long int),fp);
			printf("%lld packages\n",order);
			memcpy(send_buffer+rb , &order, sizeof(long long int));
			sendto(sockfd,send_buffer,rb+sizeof(long long int),0,pservaddr,servlen);
		}
		
		while(timeo(sockfd) == 0)
		{
			printf("%lld timeout\n", order);
			sendto(sockfd,send_buffer,rb+sizeof(long long int),0,pservaddr,servlen);
		}
		if((n = recvfrom(sockfd,ack,sizeof(long long int),0,pservaddr,&len)) <0 ){	
				printf("recvfrom error\n");
		}
		else memcpy(&check,ack,sizeof(long long int));
		
	}
	fclose(fp);
}


int main(int argc, char* argv[])
{
    int fd;
    FILE* fp;
    struct sockaddr_in servaddr,cliaddr;
    if(argc != 4){
		perror("usage:./sender <IP> <PORT> <FILE>\n");
		exit(1);
	}
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET,argv[1],&servaddr.sin_addr);
    
    if((fd=socket(AF_INET,SOCK_DGRAM,0))==-1){
    	perror("socket error");
    	exit(1);
    }
    fp = fopen(argv[3], "rb");
    if(fp == NULL){
		perror("Open file failure\n");
		exit(1);
	}
    fsend(fd,(struct sockaddr*) &servaddr,sizeof(servaddr),fp);
    
    return 0;
}



