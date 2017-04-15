#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#define BUFFER 1400

void fsend(int sockfd,struct sockaddr* pservaddr,socklen_t servlen,FILE* rfile)
{
	
	socklen_t len = servlen;
	
    int n;
    char send_buffer[BUFFER];
	char confirm[sizeof(long long int)];
	long long int order = -1;
	long long int check = -1;
	int rb;

	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 5000;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
    while(!feof(rfile))
	{
		if(order == check)
		{
			order++;
			rb =  fread(send_buffer,1,BUFFER-sizeof(long long int),rfile);
			printf("%lld packages\n",order);
			if(rb<=0) break;
			memcpy(send_buffer+rb , &order, sizeof(long long int));
			sendto(sockfd,send_buffer,rb+sizeof(long long int),0,pservaddr,servlen);
		}
		
		while((n = recvfrom(sockfd,confirm,sizeof(long long int),0,pservaddr,&len)) <0 )
		{
			if(errno == EWOULDBLOCK){
				printf("%lld timeout\n", order);
				sendto(sockfd,send_buffer,rb+sizeof(long long int),0,pservaddr,servlen);
			}
			else printf("recvfrom error\n");
			
		}
		memcpy(&check,confirm,sizeof(long long int));
	}
	fclose(rfile);
}

int main(int argc, char* argv[])
{
    int fd;
    FILE* fp;
    struct sockaddr_in servaddr,cliaddr;
	if(argc != 4){
		printf("usage:./sender <IP> <PORT> <FILE>\n");
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



