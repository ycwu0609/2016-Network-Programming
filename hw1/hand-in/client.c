#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>   //for socket()
#include <sys/socket.h> //for socket()
#include <netinet/in.h> //for socketaddr_in
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

int main(int argc, char** argv){
	struct sockaddr_in server;
	fd_set allset;
	fd_set read;
	int sockfd;
	int fdmax;
	int n, w;
		
	//./client [ip] [port]
	if(argc != 3);
    else{
        //build socket
	    sockfd = socket(AF_INET, SOCK_STREAM, 0);

        //setup data for connection
	    server.sin_family = AF_INET;
	    server.sin_port = htons(atoi(argv[2])); 
      	inet_pton(AF_INET, argv[1], &server.sin_addr.s_addr);  
      	memset(server.sin_zero, '\0', sizeof(server.sin_zero));
      	
      	//connect
      	char buf[60];
      	connect(sockfd, (struct sockaddr *)&server, sizeof(server));  
      	
      	//receive hello message
      	recv(sockfd, buf, 60, 0);
      	printf("%s\n", buf);
      	
      	FD_ZERO(&allset);
      	FD_ZERO(&read);
      	FD_SET(0, &allset);
      	FD_SET(sockfd, &allset);
      	fdmax = sockfd;
      	
      	while(1){
      	    read = allset;
      	    select(fdmax+1, &read, 0, 0, 0);
      	    for(int i=0; i<=fdmax; i++){
      	        if(FD_ISSET(i, &read)){
      	            char send_msg[1000];
      	            char rec_msg[1000];
      	            int nbytes;
      	            if(i==0){
						char *exit_msg;
      	                char *err_avoid;
						char copy[1000];
						fgets(send_msg, 1000, stdin);
						strcpy(copy, send_msg);
      	                err_avoid = strtok(copy, " ");
						
						//deal with enter with nothing
						if(!strcmp(err_avoid, "\n")) continue;
						
						if(send(sockfd, send_msg, 1000, 0) == -1){
							perror("send error");
							exit(1);
						}
						
						exit_msg = strtok(send_msg, " ");
						if(!strcmp((exit_msg =strtok(exit_msg, "\n")), "exit" )) {
							close(sockfd);
							return 0;
						}
      	            }
      	            else{
      	                nbytes = recv(sockfd, rec_msg, 1000, 0);
						rec_msg[nbytes]='\0';
      	                printf("%s\n", rec_msg);
      	                fflush(stdout);
      	            }
      	        }
      	    }
      	
      	}
      	close(sockfd);
      	return 0;
    }
}
	

