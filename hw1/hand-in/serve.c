#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>   //for socket()
#include <sys/socket.h> //for socket()
#include <netinet/in.h> //for socketaddr_in
#include <sys/select.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>


int cnt = 0;

//user
struct usernode{
    char name[20];
    struct sockaddr_in addr;
};


struct usernode client_set[100];

void user_rename(int i, char* new_name, fd_set allset, int sockfd, int sock_client){
	char rename[1000] = "[Server] You're now known as ";
	int len = strlen(new_name);
	//----------------error detection--------------------
	//new name should be 2-12 English letters
	//check if the length is between 2 and 12
	if(len<2 || len > 12) {
		if(send(i, "[Server] ERROR: Username can only consists of 2~12 English letters.", 1000, 0) == -1){
			perror("send error");
			exit(1);
		}
		return;
	}
	//check if each character is English letters
	for(int k=0; k<len;k++){
		if(!(65<=new_name[k] && new_name[k]<=90) && !(97<=new_name[k] && new_name[k]<=122)){
			if(send(i, "[Server] ERROR: Username can only consists of 2~12 English letters.", 1000, 0) == -1){
				perror("send error");
				exit(1);
			}
			return;
		}	
	}
	
	//new name can not be anonymous
	if(!strcmp(new_name,"anonymous")){
		if(send(i, "[Server] ERROR: Username cannot be anonymous.", 1000, 0) == -1){
			perror("send error");
			exit(1);
		}
		return;
	}
	
	//new name cannot be same as other clients
	for(int j=0; j<=sock_client; j++){
		// go through other clients
		if(FD_ISSET(j, &allset)){
			if(j!=sockfd && j!=i) {
				if(!strcmp(new_name, client_set[j].name)){
					char error[1000];
					strcpy(error, "[Server] ERROR: ");
					strcat(error, new_name);
					strcat(error, " has been used by others.");
					if(send(i, error, 1000, 0) == -1){
						perror("send error");
						exit(1);
					}
					return;
				}
			} 
		}
	}
	//-----------------------------------------------------------------
	
	strcat(rename, new_name);
	if(send(i, rename, 1000, 0) == -1){
		perror("send error");
		exit(1);
	}
	
	char tell_other[1000] = "[Server] ";
	strcat(tell_other, client_set[i].name);
	strcat(tell_other, " is known as ");
	strcat(tell_other, new_name);
	strcpy(client_set[i].name, new_name);
	for(int j=0; j<=sock_client; j++){
		// send to other clients
		if(FD_ISSET(j, &allset)){
			if(j!=sockfd && j!=i) {
				if(send(j, tell_other, 1000, 0) == -1){
					perror("send error");
					exit(1);
				}
			} 
		}
	}
}


void broadcast(int i, char* text, fd_set allset, int sockfd, int sock_client){
	char buffer[1000]= "[Server] ";
	char *tmp = strtok(text, "\n");
	strcat(buffer, client_set[i].name);
	strcat(buffer, " ");
	strcat(buffer, tmp);
	// send to other clients
	for(int j=0; j<=sock_client; j++){
		if(FD_ISSET(j, &allset)){
			if(j!=sockfd ) {
				if(send(j, buffer, 1000, 0) == -1){
					perror("send error");
					exit(1);
				}
			}
		}
	}

}

void private_message(int i, char* text, fd_set allset, int sockfd, int sock_client){
	char buf[1000] = {0};
	if(!strcmp(client_set[i].name, "anonymous")){
		send(i, "[Server] ERROR: You are anonymous.", 1000, 0);
		return;
	}
	strcpy(buf, client_set[i].name);
	strcat(buf, " tell you ");
	//tell
	char* token = strtok(text, " ");
	//name
	char client_name[20];
	token = strtok(NULL, " ");
	strcpy(client_name, token);
	if(!strcmp(client_name, "anonymous")){
		if(send(i, "[Server] ERROR: The client to which you sent is anonymous.", 1000, 0) == -1){
			perror("send error");
			exit(1);
		}
		return;
	}
	//the text
	token = strtok(NULL, "\n");
	strcat(buf, token);
	//printf("buf = %s\n", buf);
	//send
	for(int j=0;j<=sock_client;j++){
		if(FD_ISSET(j, &allset)){
			if(!strcmp(client_set[j].name, client_name) && j!=sockfd && j!=i){
				if(send(j, buf, 1000, 0) == -1){
					perror("send error");
					exit(1);
				}
				if(send(i, "[Server] SUCCESS: Your message has been sent.", 1000,0) == -1){
					perror("send error");
					exit(1);
				}
				return;
			}
		}
	}
	if(send(i, "[Server] ERROR: The receiver doesn't exist.", 1000,0) == -1){
		perror("send error");
		exit(1);
	}
				
	

}


void who(int i, fd_set allset, int sockfd, int sock_client){
	//printf("%s\n", text);
	for(int k=0;k<=sock_client;k++){
		if(FD_ISSET(k, &allset) && k!=sockfd){
			char who_msg[1000] = {0};
			who_msg[999]='\0';
			strcpy(who_msg, "[Server] ");
			strcat(who_msg, client_set[k].name);
			strcat(who_msg, " ");
			strcat(who_msg, inet_ntoa(client_set[k].addr.sin_addr));
			strcat(who_msg, "/");
			char port[5];
			sprintf(port,"%d", ntohs(client_set[k].addr.sin_port));
			strcat(who_msg, port);
			if(k==i) strcat(who_msg, " <-me");
			if(send(i, who_msg, 1000, 0) == -1){
				perror("send error");
				exit(1);
			}
		}
	}
}



int main(){
	fd_set allset, read;
	int sockfd;
	struct sockaddr_in server;
	struct sockaddr_in client;
	socklen_t len;
	int sock_client;

	//------------------------initialozation---------------------------
	FD_ZERO(&allset);
	FD_ZERO(&read);

	//------------------------setup connection--------------------------------
	//build socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		perror("socket error");
		exit(1);
	}

	//setup socket
	server.sin_family = AF_INET;
	server.sin_port = htons(12345);
	server.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY: Return self IP address
	memset(server.sin_zero, '\0', sizeof(server.sin_zero));	
	if(bind(sockfd, (struct sockaddr*) &server, sizeof(struct sockaddr)) == -1){
		perror("bind error");
		exit(1);
	}
	printf("[Info] Binding...\n");

	
	//waiting for TCP clients
	if(listen(sockfd, 5) == -1){
		perror("listen error");
		exit(1);
	}
	printf("[Info] listening...\n");
	FD_SET(sockfd, &allset);	
	sock_client = sockfd;
	//-------------------------------------------------------------------------
	while(1){
		read = allset;
		if(select(sock_client+1, &read, 0, 0, 0)==-1){
		 	perror("select error");
		 	exit(1);
		}
		for(int i=0;i<=sock_client;i++){
			if(FD_ISSET(i, &read)){ //check if i is in the set
				if(i == sockfd){
					//-----accept clients-----
					int new;
					len = sizeof(client);
					new = accept(sockfd, (struct sockaddr*) &client, &len);
					if(new == -1){
						perror("accept error");
						exit(1);
					}
					FD_SET(new, &allset);
					if(new > sock_client) sock_client = new;
					//-----add the client to the array-----
					struct usernode new_user;
					strcpy(new_user.name, "anonymous");
					new_user.addr = client;
					client_set[new] = new_user;
					cnt++;
					//-----------save parameters------------
					char *new_address = inet_ntoa(client.sin_addr);
					char new_port[5];
					sprintf(new_port,"%d", ntohs(client.sin_port));
					printf("New client!\nNow has %d clients\n",cnt);
					fflush(stdout);
					//---------build hello message-------
					
					char hello[60];
					strcpy(hello, "[Server] Hello, anonymous! From: ");
					strcat(hello, new_address);
					strcat(hello, "/");
					strcat(hello, new_port);
					//----sent to new client-----
					if(send(new, hello, 60, 0) == -1){
						perror("send error");
						exit(1);
					}
					// send to other clients
					for(int j=0; j<=sock_client; j++){
						if(FD_ISSET(j, &allset)){
							if(j!=sockfd && j!=new){
								if( send(j, "[Server] Someone is coming", 28, 0) == -1){
									perror("send error");
									exit(1);
								}
							}
						}
					}
				}
				else{
					int bytes;
					char buf[1000] = {0};
					//----receive clients messages----
					if(bytes = recv(i, buf, 1000, 0)>0){
						char alltext[1000] = {0};
						char *token, *who_token;
						strcpy(alltext, buf);
						token = strtok(buf," "); //seperate command and argument
						//name <new_name>
						if(!strcmp(token, "name")){
							char new_name[20];
							token = strtok(NULL, "\n");
							strcpy(new_name, token);
							user_rename(i, new_name, allset, sockfd, sock_client);
							continue; 
						}
					
						//yell
						if(!strcmp(token, "yell")){
							broadcast(i, alltext, allset, sockfd, sock_client);
							continue;
						}
						
						//tell <USERNAME><MESSAGE>
						if(!strcmp(token, "tell")){
							private_message(i, alltext, allset, sockfd, sock_client);
							continue;
						}
					
						//who
						//deal with "   who   "
						if(!strcmp((who_token = strtok(token, "\n")), "who")){
							who(i, allset, sockfd, sock_client);
							continue;
						}
						
						//exit
						//deal with "   exit  "
						if(!strcmp((who_token = strtok(token, "\n")), "exit")) continue;
					
						if(send(i,"[Server] ERROR: Error command.", 50,0) == -1){
							perror("send error");
							exit(1);
						}
					
						
					}
					else{
						//----build offline message----
						char offline[50] = "[Server] ";
						strcat(offline, client_set[i].name);
						strcat(offline, " is offline.");
						for(int j=0; j<=sock_client; j++){
							// send to other clients
							if(FD_ISSET(j, &allset)){
								if(j!=sockfd && j!=i) {
									if(send(j, offline, 50, 0) == -1){
										perror("send error");
										exit(1);
									}
								}
							}
						
						}
						cnt--;
						printf("%s\nNow has %d clients.\n",offline, cnt);
						fflush(stdout);
						close(i);
						FD_CLR(i, &allset);
					}
				}
			}
		}


	}
	
	return 0;	
}
