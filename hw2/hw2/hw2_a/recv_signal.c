#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#define BUFSIZE 1400

void frecv(int sockfd, struct sockaddr* cliaddr, socklen_t clen,FILE* wfile)
{
    int nbyte;
    socklen_t len = clen;
    char sendbuf[BUFSIZE];
	long int order = -1;
	long int check;
	int wb;
	char ack[sizeof(int)];
    while(1)
    {
        if((nbyte = recvfrom(sockfd,sendbuf,BUFSIZE,0,cliaddr,&len))<0) {
        	perror("recvfrom error");
			exit(1);
		}
		else {
   			memcpy(&check,sendbuf+nbyte-sizeof(long int),sizeof(long int));
			if(check-1 == order){
				order++;
				wb = fwrite(sendbuf,nbyte-sizeof(long int),1,wfile);
				memcpy(ack,&order,sizeof(long int));
				printf("%ld packages\n",order);
				sendto(sockfd,ack,sizeof(long int),0,cliaddr,len);
			}
			else sendto(sockfd,ack,sizeof(long int),0,cliaddr,len);
			
			if(nbyte != BUFSIZE) break;
    	}
    }
	fclose(wfile);
}

int main(int argc, char* argv[])
{
    int sockfd;
    struct sockaddr_in  servaddr,cliaddr;
    FILE* wfile;
	if(argc != 3){
		perror("usage:./receiver <Port> <filename>\n");
		exit(1);
	}


    if((sockfd = socket(AF_INET,SOCK_DGRAM,0))<0){
        perror("socket error\n");
        exit(1);
	}
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if(bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0){
        perror("bind error\n");
        exit(1);
    }
	wfile = fopen(argv[2], "wb");	
	if(wfile == NULL){
		perror("Open file failure\n");
		exit(1);
	}
	frecv(sockfd, (struct sockaddr*) &cliaddr, sizeof(cliaddr),wfile);

    return 0;
}


