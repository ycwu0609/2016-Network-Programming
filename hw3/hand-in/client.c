#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAXLINE 1400 

struct client{
	int command, trans;
	char name[100];
	char filename[100];
	char fsize[100];
	int dflag;
}client;

void do_command(struct sockaddr* servaddr, socklen_t servlen){
	fd_set rset, wset;
	int maxfd, n_ready;
	int read_len=0;
	char read_buff[MAXLINE];	
	int send_len=0;
	char send_buff[MAXLINE];
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	
	while(1){
		
		FD_SET(client.command,&rset);
		FD_SET(STDIN_FILENO,&rset);
		maxfd = client.command;
		
		n_ready = select(maxfd+1,&rset,NULL,NULL,NULL);
		
		if(FD_ISSET(client.command,&rset)){ // from server
			//printf("from server\n");
			read_len = read(client.command,read_buff,sizeof(read_buff));
			if(read_len<0){
				perror("Cli command Read error:");
				return;
			}
			else if(read_len==0){
				printf("Server close.\n");
				close(client.command);
				FD_CLR(client.command,&rset);
				return;
			}
			else{
				read_buff[read_len]=0;
				if(read_buff[0]=='/'){
					char filename[100];
					sscanf(read_buff,"/put %s %s\n",client.filename,client.fsize);
					//printf("download info: name %s size %s.\n",cli.filename,cli.fsize);
					FD_SET(client.trans,&rset);
					client.dflag=1;
				}
				else printf("%s",read_buff);
			}
		}
		if(FD_ISSET(STDIN_FILENO,&rset)){ // from stdin
			read_len = read(STDIN_FILENO,read_buff,sizeof(read_buff));
			read_buff[read_len]=0;
			
			if(!strcmp(read_buff,"/exit\n")){
				close(client.trans);//shutdown(cli.trans,SHUT_WR);
				shutdown(client.command,SHUT_WR);
				FD_CLR(client.trans,&rset);
			}
			else{
				char command[6], content[1000];
				sscanf(read_buff,"/%s %s\n",command,content);
				if(!strcmp(command,"sleep")){
					int sec = atoi(content);
					printf("Client starts to sleep\n");
					int count=0;
					while(count<sec&&sec!=0){
						sleep(1);
						count++;
						printf("Sleep %d\n",count);
					}
					printf("client wakes up\n");
				}
				else if(!strcmp(command,"put")){
					printf("Upload file: %s\n",content);
					printf("Progress : [");
					fflush(stdout);
					
					FILE* fp;
					fp = fopen(content,"rb");
					/*get file size*/
					fseek(fp,0,SEEK_END);
					int fsize = ftell(fp);
					rewind(fp);
					read_buff[read_len-1]=0;
					char file_size[100];
					sprintf(file_size,"%d",fsize);//itoa(fsize,file_size,10);
					sprintf(send_buff,"%s %s\n",read_buff,file_size);
					write(client.command,send_buff,strlen(send_buff));
					/*read file*/
					int fread_len;
					char fread_buff[MAXLINE];
					int size = 0;
					float prog = (float)fsize/20.0;
					while(size<fsize){
						fread_len = fread(fread_buff,sizeof(char),((fsize-size)>MAXLINE? MAXLINE: fsize-size),fp);
						write(client.trans,fread_buff,((fsize-size)>MAXLINE? MAXLINE: fsize-size));
						size+= fread_len;
						while((float)size>prog){
							printf("#");
							prog+=(float)fsize/20.0;
							fflush(stdout);
						}
					}
					fclose(fp);	
					printf("#]\nUpload %s complete!\n",content);
					shutdown(client.trans,SHUT_WR);
					FD_SET(client.trans,&rset);
					//close(cli.trans);
					//continue;
				}
			}
		}
		if(FD_ISSET(client.trans,&rset)){
			if(client.dflag){			
				// download information
				printf("Download file: %s\n",client.filename);
				printf("Progress : [");
				fflush(stdout);
				
				//strcat(cli.filename,"_d");
				FILE* outfp = fopen(client.filename,"wb");
				int fsize = atoi(client.fsize);
				//printf("fsize: %d\n",fsize);
				int size=0;//int size=fsize;
				float prog = (float)fsize/20.0;
				while(size<fsize/*size>0*/){
					read_len = read(client.trans,read_buff,(fsize-size>MAXLINE? MAXLINE: fsize-size));
					//printf("Download progress: size %d get %d\n",size,read_len);
					fwrite(read_buff,sizeof(char),read_len,outfp);
					size+=read_len;//size-=read_len;
					while((float)size>prog){
						printf("#");
						prog+=(float)fsize/20.0;
						fflush(stdout);
					}
					/*if(size==0){
						fclose(outfp);
						cli.dflag=0;
						printf("download done\n");
						break;
					}*/
				}
				fclose(outfp);
				client.dflag=0;
				printf("#]\nDownload %s Complete!\n",client.filename);
				close(client.trans);
				FD_CLR(client.trans,&rset);
				//connect again
				client.trans = socket(AF_INET,SOCK_STREAM,0);
				connect(client.trans,servaddr,servlen);
				continue;
			}
			
				read_len = read(client.trans,read_buff,MAXLINE);
				if(read_len==0){
					//printf("transfd close.\n");
					close(client.trans);
					FD_CLR(client.trans,&rset);
					//connect again
					client.trans = socket(AF_INET,SOCK_STREAM,0);
					connect(client.trans,servaddr,servlen);
				}
			
			/*
			FD_CLR(cli.trans,&rset);
			close(cli.trans);
			cli.trans = socket(AF_INET,SOCK_STREAM,0);
			connect(cli.trans,servaddr,servlen);*/
		}
	}
	
}
int connect_to_server(struct sockaddr* servaddr, socklen_t servlen){
	
	if((client.command = socket(AF_INET,SOCK_STREAM,0))<0){
		perror("Socket error: commandsocket");
		return -1;
	}
	if(connect(client.command,servaddr,servlen)<0){
		perror("Connect error:");
		return -1;
	}
	char send_buff[MAXLINE];
	sprintf(send_buff,"name %s\n",client.name);
	write(client.command,send_buff,strlen(send_buff));
	if((client.trans = socket(AF_INET,SOCK_STREAM,0))<0){
		perror("Socket error: transmitsocket");
		return -1;
	}
	if(connect(client.trans,servaddr,servlen)<0){
		perror("Connect error:");
		return -1;
	}
	
	//printf("connect done! connectfd: %d\n",cli.command);

	return client.command;
}

int main(int argc, char* argv[])
{
	if(argc!=4){
		printf("Usage: ./client <IP> <PORT> <USERNAME>\n");
		return 0;
	}
		
	struct sockaddr_in servaddr;
	memset(&servaddr,0,sizeof(servaddr));
	inet_pton(AF_INET,argv[1],&servaddr.sin_addr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons( atoi(argv[2]) );
	strcpy(client.name,argv[3]);
	
	if(connect_to_server((struct sockaddr*)&servaddr, sizeof(servaddr))<0)
		return -1;
	
	do_command((struct sockaddr*)&servaddr,sizeof(servaddr));	
	
	//close(cli.trans);
	//close(cli.command);
	return 0;
}



