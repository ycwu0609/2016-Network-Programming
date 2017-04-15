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

struct file{
	char filename[100];
	int filesize;
};

struct client{
	/*socket*/
	int command;
	int trans;
	//int trans_num;
	int cnt;
	//char name[100];
	char ip[INET_ADDRSTRLEN];
	/*user*/
	int id; // correspond to userlist
	/*current file*/
	struct file filelist[100];
	int fnum; // how many file this client have
	FILE* up_file;
	int up_fsize; // total
	int up_get;  // be sent
	int up_flag;
	FILE* down_file;
	int down_fsize;
	int down_sent;
	int down_flag;
} client[FD_SETSIZE/2];

struct usernode{
	char name[100];
	/*filelist*/
	int fnum; // how many file this user have
	struct file filelist[100];
	int nflag;
	int online;
}userlist[FD_SETSIZE/2];

int main(int argc,char** argv){
	int num = 0;
	if(argc!=2){
		perror("Usage: ./server <PORT>");
		exit(0);
	}
	
	int listenfd;
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[1]));
	servaddr.sin_addr.s_addr = INADDR_ANY;
	
	// create socket
	if((listenfd=socket(AF_INET,SOCK_STREAM,0))<0){
		perror("listen error");
		exit(1);
	}
	// bind
	if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
		perror("bind error");
		exit(1);
	}
	
	// start listening
	if(listen(listenfd,10)<0){
		perror("listen error");
		exit(1);
	}
	
	//  declare variable: fd, client
	fd_set allset;
	fd_set rset;
	fd_set wallset;
	fd_set wset;
	int i, maxi, maxfd, n_ready, unum;
	//struct client cli[FD_SETSIZE];
	maxfd = listenfd;
	maxi = -1;
	unum = -1;
	FD_ZERO(&allset);
	FD_ZERO(&wallset);
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(listenfd,&allset);
	memset(client,0,sizeof(client));
	for(i=0;i<FD_SETSIZE/2;i++){
		client[i].command=-1;
		client[i].trans=-1;
		client[i].id = -1;
		client[i].cnt =-1;
		//client[i].trans_num = 0;
	}
	memset(userlist,0,sizeof(userlist));
	
	while(1){
		rset = allset;
		wset = wallset;
		n_ready = select(maxfd+1,&rset,&wset,NULL,NULL);
		//printf("selected %d\n",n_ready);
		int client_fd;
		int recv_len;
		char recv_buff[MAXLINE+1];
		memset(recv_buff,0,sizeof(recv_buff));
		size_t send_len;
		char send_buff[MAXLINE];
		memset(send_buff,0,sizeof(send_buff));
		
		if(FD_ISSET(listenfd,&rset)){
			// accept client
			if((client_fd = accept(listenfd,NULL,NULL))<0){
				perror("Accept error:");
				break;
			}
			/*set non-blocking*/
			int flag=fcntl(client_fd,F_GETFL,0);
			fcntl(client_fd,F_SETFL,flag|O_NONBLOCK);
			//printf("client in\n");
			// check source
			struct sockaddr_storage c;
			memset(&c,0,sizeof(c));
			int len = sizeof(c);
			getpeername(client_fd,(struct sockaddr*)&c,&len);
			struct sockaddr_in *s = (struct sockaddr_in *) &c;
			
			char temp_ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET,&s->sin_addr,temp_ip,sizeof(temp_ip));
			//printf("%s\n",temp_ip);
			FD_SET(client_fd,&allset);			
			
			for(i=0;i<=maxi;i++){
				if(client[i].command<0) continue;
				//printf("%d %d %s,%s.\n",i,cli[i].command,cli[i].ip,temp_ip);
				//if(strcmp(client[i].ip,temp_ip)==0){		
				if(client[i].cnt==num){
					client[i].trans = client_fd;
					FD_SET(client[i].trans,&allset);
					num++;
					break;
				}
			}
			
			if((i>maxi)||maxi==-1){
				//printf("no same source.\n");
				for(i=0;i<FD_SETSIZE/2;i++){
					// find available
					if(client[i].command<0){
						//printf("new client\n");
						client[i].command = client_fd;
						client[i].cnt = num;
						//strcpy(client[i].ip,temp_ip);
						FD_SET(client[i].command,&allset);
						//sprintf(send_buff,"Hi! You're client %d\n",i);
						//write(cli[i].command,send_buff,strlen(send_buff));
						break;
					}
				}
				if(i==FD_SETSIZE/2){
					perror("Full.\n");
					break;
				}
			}
			// adjust
			//num++;
			if(maxi<i) maxi = i;
			if(maxfd<client_fd) maxfd = client_fd;
			
		}
		
		int cmd,tms;
		for(i=0;i<=maxi;i++){
			cmd = client[i].command;
			tms = client[i].trans;
			//printf("cmd %d tms %d\n",cmd,tms);
			if(cmd<0) continue;
			if(FD_ISSET(cmd,&rset)){
				recv_len = read(cmd,recv_buff,sizeof(recv_buff));
				recv_buff[recv_len] = 0;
				if(recv_len<0){
					perror("Read error:");
					close(cmd);
					FD_CLR(cmd,&allset);
					memset(&client[i],0,sizeof(struct client));
					client[i].command = -1;
					client[i].trans = -1;
					//client[i].trans_num = 0;
					client[i].id = -1;
					continue;
				}
				else if(recv_len==0){
					printf("Client sign out\n");
					close(cmd);
					FD_CLR(cmd,&allset);
					memset(&client[i],0,sizeof(struct client));
					client[i].command = -1;
					client[i].trans = -1;
					client[i].cnt = -1;
					//client[i].trans_num = 0;
					client[i].id = -1;
					if(i==maxi) maxi--;
					continue;
				}
				else{
					if(recv_buff[0]=='/'){
						char file_size[10];
						char filename[20];
						sscanf(recv_buff,"/put %s %s\n",filename,file_size);
						//printf("Upload info: filename %s size %s\n",filename,file_size);
						
						FD_SET(client[i].trans,&allset);
						FD_SET(client[i].trans,&rset);
						
						//strcat(filename,"_u");
						strcpy(client[i].filelist[client[i].fnum].filename,filename);
						client[i].filelist[client[i].fnum].filesize = atoi(file_size);
						printf("Client %s is going to upload %s\n",userlist[client[i].id].name,client[i].filelist[client[i].fnum].filename);
						client[i].up_file = fopen(filename,"wb");
						client[i].up_fsize = atoi(file_size);
						client[i].up_flag = 1;
						
					}
					else{
						if(recv_buff[0]=='n'){
							//printf("%s",recv_buff);
							char cliname[10];
							sscanf(recv_buff,"name %s\n",cliname);
							
							int j;
							for(j=0;j<=unum;j++){
								if(userlist[j].name[0]==0) continue;
								if(!strcmp(cliname,userlist[j].name)){
									client[i].id = j;
									
									userlist[j].online++;
									break;
								}
							}
							//printf("j: %d unum: %d\n",j,unum);
							if(j>unum||unum==-1){
								for(j=0;j<FD_SETSIZE/2;j++){
									if(userlist[j].name[0]==0){
										client[i].id = j;
										strcpy(userlist[j].name,cliname);
										userlist[j].online=1;
										break;
									}
								}
							}
							if(unum<j) unum=j;
							//printf("User %d: %s\n",j,userlist[cli[i].id].name);
							sprintf(send_buff,"Welcome to the dropbox-like server! : %s\n",userlist[client[i].id].name);
							write(cmd,send_buff,strlen(send_buff));
						}
					}
				}
				//if(--n_ready<=0) break; // if sock = -1, core dump?
			}
		
			if(tms<0) continue;
			
			/*check newfile*/
			if(client[i].id != -1 && client[i].fnum < userlist[client[i].id].fnum){
				printf("check new file\n");
				//printf("client:%i uf: %d df: %d\n",i,client[i].up_flag,client[i].down_flag);
				if(client[i].up_flag==0 && client[i].down_flag==0){
					/*set download info*/
					client[i].down_flag = 1;
					client[i].down_file = fopen(userlist[client[i].id].filelist[client[i].fnum].filename,"rb");
					if(client[i].down_file == NULL) printf("file open fail.\n");
					printf("Client have %d User have %d\n",client[i].fnum,userlist[client[i].id].fnum);
					printf("Put %s\n",userlist[client[i].id].filelist[client[i].fnum].filename);
					/*get file size*/
					client[i].down_fsize = userlist[client[i].id].filelist[client[i].fnum].filesize;
					char file_size[100];
					sprintf(file_size,"%d",client[i].down_fsize);//itoa(fsize,file_size,10);
					sprintf(send_buff,"/put %s %s\n",userlist[client[i].id].filelist[client[i].fnum].filename,file_size);
					write(client[i].command,send_buff,strlen(send_buff));
					FD_SET(tms,&wallset);
					//break;
					//n_ready++;
				}
			}
			if(FD_ISSET(tms,&rset)){
				//printf("tms\n");
				recv_len = read(tms,recv_buff,(client[i].up_fsize>MAXLINE? MAXLINE:client[i].up_fsize));
				if(recv_len<0) perror("");
				else if(recv_len==0){
					printf("%d transmission done\n",i);
					FD_CLR(tms,&allset);
					FD_CLR(tms,&wallset);
					close(tms);
					client[i].trans = -1;
					if(client[i].up_flag){
						client[i].up_get = 0;
						client[i].up_fsize = 0;
						client[i].up_flag = 0;
					}
					else if(client[i].down_flag){
						//printf("here!\n");
						client[i].down_sent = 0;
						client[i].down_fsize = 0;
						client[i].down_flag = 0;
					}
				}
				else{
					//printf("Upload progress: size %d\n",cli[i].up_fsize);
					recv_buff[recv_len]=0;
					fwrite(recv_buff,sizeof(char),recv_len,client[i].up_file);
					client[i].up_fsize-=recv_len;
					client[i].up_get+=recv_len;
					if(client[i].up_fsize==0){
						fclose(client[i].up_file);
						/*upload done*/
						userlist[client[i].id].filelist[userlist[client[i].id].fnum++] = client[i].filelist[client[i].fnum];
						client[i].fnum++;
						printf("User %s now have %d files.\n",userlist[client[i].id].name,userlist[client[i].id].fnum);
					}
				}
			}
						
			if(FD_ISSET(tms,&wset)){
				//printf("tms w\n");
				/*read file*/
				int fread_len;
				char fread_buff[MAXLINE];
				int size = client[i].down_sent;	
				if(size<client[i].down_fsize){
					fread_len = fread(fread_buff,sizeof(char),((client[i].down_fsize-size)>MAXLINE? MAXLINE: client[i].down_fsize-size),client[i].down_file);
					printf("%d\n",fread_len);
					write(tms,fread_buff,((client[i].down_fsize-size)>MAXLINE? MAXLINE: client[i].down_fsize-size));
					size+= fread_len;
					client[i].down_sent = size;
				}
				if(client[i].down_sent==client[i].down_fsize){
					//printf("file end.\n");
					fclose(client[i].down_file);
					//FD_CLR(tms,&allset);
					FD_CLR(tms,&wallset);
					//close(tms);
					//cli[i].trans = -1;
					client[i].down_fsize = 0;
					client[i].down_sent = 0;
					//cli[i].down_flag = 0;
				
					client[i].filelist[client[i].fnum] = userlist[client[i].id].filelist[client[i].fnum];
					client[i].fnum++;
					printf("Client%d %s %d User %d.\n",i,userlist[client[i].id].name,userlist[client[i].id].fnum,userlist[client[i].id].fnum);
					FD_SET(tms,&rset);
				}
			
			}
		}
	}
	close(listenfd);
	return 0;
}
