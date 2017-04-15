#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#define BUFFER 1400

static void sig_alrm(int sig){
	return;
}

void fsend(int sockfd,struct sockaddr* pservaddr,socklen_t servlen,FILE* rfile)
{

	signal(SIGALRM,sig_alrm);	/////
	siginterrupt(SIGALRM,1);	/////
	socklen_t len = servlen;
	
    int n;
    char send_buffer[BUFFER];
	char confirm[sizeof(long long int)];
	long long int order = -1;
	long long int check = -1;
	int rb;
    while(!feof(rfile))
	{
		if( order == check)
		{
			//printf("new\n");
			order++;
			rb =  fread(send_buffer,1,BUFFER-sizeof(long long int),rfile);
			printf("%lld packages\n",order);
			//if(rnum<=0) break;
			memcpy(send_buffer+rb , &order, sizeof(long long int));
			sendto(sockfd,send_buffer,rb+sizeof(long long int),0,pservaddr,servlen);
		}
		
		alarm(1);	////
		while((n = recvfrom(sockfd,confirm,sizeof(long long int),0,pservaddr,&len)) <0 )
		{
			if(errno == EINTR){
				alarm(1);
				printf("%lld timeout\n", order);
				sendto(sockfd,send_buffer,rb+sizeof(long long int),0,pservaddr,servlen);
			}
			else
				printf("recvfrom error\n");
		}
		alarm(0);	/////
		memcpy(&check,confirm,sizeof(long long int));
	}
	fclose(rfile);
}


int main(int argc, char* argv[])
{
    int fd;
    struct sockaddr_in servaddr,cliaddr;
    FILE* fp;
    
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


