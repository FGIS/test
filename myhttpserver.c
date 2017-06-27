
#define	BUF_LEN	128
#define SUCCESS "HTTP/1.1 200 Ok\n"
#define ERROR "HTTP/1.0 404 Not Found\n"
#define BAD "TTTP/1.0 400 Bad Request\n"
#define MESS_404 "<html><body><h1>FILE NOT FOUND</h1></body></html>"
#define DIRRES "NO index.html Found. Here is the files in this directory:\n"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <netdb.h>
#include <dirent.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>


int waitingtime, islog=1, iswait, isdebug=0;
int thread_num=5;
//thread_pool* pool;


struct request
{
	int socketfd;
	char *file_name;
	unsigned int ip;
	char time_arrival[128];
	char buffer[256];
	int file_size;
};

void usage(){
 fprintf(stderr, "usage:  -h host -p port\n");
 fprintf(stderr, "usage:  -s [-p port]\n");
 exit(1);
}

void request_process(struct request req){
	
	// while(1){
    //get the current timestamp!
    time_t curtime=time(NULL);
	struct tm tm=*gmtime(&curtime);
	char serve_time[128];
	strcpy(serve_time,asctime(&tm));
	//strftime(serve_time, sizeof(serve_time), "[%d/%b/%Y : %H:%M:%S %z]", &tm);
    char *status=NULL;
    const char *fname = NULL;
    char *contenttype = NULL;
    char *method = NULL;
    char fsize[128];
    char output[8192];
    struct stat fileStat;
    int requestfile;
    char hostname[128];
    char buf[1024];
    char file_content[1024];
    size_t nread;

    char bytes[4];
    unsigned int remote_ip=req.ip;
    bytes[0] = remote_ip & 0xFF;
	bytes[1] = (remote_ip >> 8) & 0xFF;
	bytes[2] = (remote_ip >> 16) & 0xFF;
	bytes[3] = (remote_ip >> 24) & 0xFF;
    fname=req.file_name;

    if(stat(fname, &fileStat)< 0) 
		status=ERROR;
	else
		status=SUCCESS;
	
	if(islog==1){
		if(isdebug==0){
			FILE * file_log=fopen("log.txt","a");
			fprintf(file_log,"%d.%d.%d.%d  -  ", bytes[0], bytes[1], bytes[2], bytes[3]);
			fprintf(file_log,"%s\b%s\b%s\b%s\b%d\n",req.time_arrival,serve_time,req.buffer,status, req.file_size);
			fclose(file_log);
		}
		else{
			printf("%d.%d.%d.%d  -  ", bytes[0], bytes[1], bytes[2], bytes[3]);
			printf("%s\b%s\b%s\b%s\b%d\n",req.time_arrival,serve_time,req.buffer, status, req.file_size);
		}
	}

    gethostname(hostname,sizeof(hostname));
    //getdomainname(hostname,sizeof(hostname));   	
 	memset(output, 0, sizeof(output));
 	//strcpy(output,"charest=UTF-8 \r\n");
 	method=strtok(req.buffer," ");
 	//requestfile=open();
 	//strtok(in_buf, " ");
 	snprintf(fsize,128,"%d",req.file_size);
   // printf("method: %s \n",method);
    //printf("File name:%s\n",fname);

    //If the request is for a folder!
    if(strstr(fname,".")==NULL ){
      chdir(fname);
      FILE *file=fopen("index.html","r");      
      if(file)
      	fname="index.html";
      else{
      	 DIR *dr=opendir(".");
         struct dirent *dir;
         if(dr){
         	send(req.socketfd,DIRRES, sizeof(DIRRES),0);
    	    while((dir=readdir(dr))!=NULL){
    	    	char res[128];
    	    	snprintf(res,128,"%s\n",dir->d_name);
    	    	//if(res[0]==".") continue;
    	    	printf("%s\n",dir->d_name);         	        
    	        send(req.socketfd, res, sizeof(res), 0);
    	    }
    	closedir(dr);
    	close(req.socketfd);
    }
    }
}

	if(stat(fname, &fileStat)< 0){             //the file do not exist 
		status=ERROR;
		strcat(output,ERROR);
		//strcat(output,"\n");
		perror("no file!");
		send(req.socketfd, output, sizeof(output), 0);
	}
	else{                                 //get the file 
		//const struct timespec lmodified=fileStat.st_mtimespec;
		strcat(output,SUCCESS);
		strcat(output,"Date:");
		strcat(output,serve_time);
		strcat(output,"Server:");
		strcat(output,hostname);
		strcat(output,"\n");
		strcat(output,"Last-Modified:");
		strcat(output,asctime(gmtime(&fileStat.st_mtime)));
		strcat(output,"Content-Length:");
		strcat(output,fsize);
		strcat(output,"\n");
		if(strstr(fname, ".jpg") != NULL || strstr(fname, ".gif") != NULL){
			 	contenttype="image/gif\n";
			 	strcat(output,"Content-Type:");
		        strcat(output,contenttype);

		        if(strstr(method,"HEAD")!= NULL){
			        send(req.socketfd,output,sizeof(output),0);
			        close(req.socketfd);
		        }
		        send(req.socketfd,output,sizeof(output),0);
		        FILE *fp =fopen(fname,"rb");
		        //FILE *newfp= fopen("mytest.jpg","ab");
		        while((nread=fread(buf, 1, sizeof(buf), fp)) > 0){
			    	// //strcat(output,buf);
			    	 //fwrite(buf, 1, sizeof(buf), newfp);
			    	 send(req.socketfd,buf,sizeof(buf),0);
		         }
		}
	    else if( strstr(fname,".html") != NULL || strstr(fname,".txt")!= NULL){
	        	contenttype="html/txt\n";
	        	strcat(output,"Content-Type:");
		        strcat(output,contenttype);
		        if(strstr(method,"HEAD")!= NULL){
			        send(req.socketfd,output,sizeof(output),0);
			        close(req.socketfd);
		        }
		        send(req.socketfd,output,sizeof(output),0);
		        FILE *fp =fopen(fname, "rb");
			    while(fread(buf, 1, sizeof(buf), fp) > 0){
			        send(req.socketfd,buf,sizeof(buf),0);
			    }
	        }
		}
	close(req.socketfd);
}


void Listen_Thread(void * sockfd){

	unsigned int sock_fd = (unsigned int*)sockfd;
	char temp_buf[128];
	struct sockaddr_in cli_addr;
	socklen_t cli_len = sizeof(cli_addr);
	struct request ac_request;
	unsigned int remote_ip=cli_addr.sin_addr.s_addr;
	char *file_name = NULL;
	struct stat fStat;
	listen(sock_fd,1);


    while(1){

	fprintf(stderr, "Entering accept() waiting for connection.\n");
	int new_socket= accept(sock_fd, (struct sockaddr *) &cli_addr, &cli_len);
	
	if(new_socket<0)
		perror("Can not accept!");

    memset(temp_buf, 0, sizeof(temp_buf));
    
    if(recv(new_socket,temp_buf,sizeof(temp_buf), 0)<0){
    	perror("can not recv!");
    }
    else{
    	strcpy(ac_request.buffer,temp_buf);
    	temp_buf[strlen(temp_buf)-1] = 0;
    	printf("%s\n", temp_buf);
    	printf("i got the connection: %s \n", temp_buf);
    	strtok(temp_buf, " \t\n");
		file_name = strtok(NULL, " \t");		
    }
    file_name++; //remove the first char "/" from the file name to get the relative path
    if(stat(file_name, &fStat)<0)
    	ac_request.file_size = 0;
    else
    	ac_request.file_size = fStat.st_size;
	//get current time in unix
	time_t curtime=time(NULL);
	struct tm tm=*gmtime(&curtime);
	char atime[128];
	strcpy(atime,asctime(&tm));

    ac_request.file_size = fStat.st_size;
	ac_request.socketfd= new_socket;
	ac_request.file_name= file_name;
	strcpy(ac_request.time_arrival,atime);
	ac_request.ip=remote_ip;
    request_process(ac_request);
	//thread_pool_add_work(pool, test, (void*)ac_request);
    }
	
	//  //this is a test for calling the request_process!
	
}


int main(int argc, char *argv[]){

	int socket_fd, new_socketfd, ch;
	socklen_t len;
	char *host = NULL;
	char *port = NULL;
	char *sched = "FCFS";
	char *cdir =NULL;

	pthread_t thread_listener, thread_scheduler;

	struct sockaddr_in severv_addr,client_addr;
	struct servent *se;
	//struct servent *se;
	
	int isdir=0;

	int thread_num = 4;
	port="8010";
	//char *port_num = 8080;
	extern char *optarg;
	extern int optind;

	while ((ch = getopt(argc, argv, "hdit:p:n:r:s:")) != -1)
		switch(ch) {
			case 'h':
				usage();		/* print address in output */
				break;
			case 'd':
				isdebug=1;
				break;
			case 'i':
			    islog = 1;
			case 't':
			    iswait = 1;
			    waitingtime = atoi(optarg);
			    break;
			case 'p':
				port = optarg;
				break;
			case 'n':
			    thread_num = atoi(optarg);
                break;
            case 'r':
                isdir=1;
                cdir=optarg;
                break;
            case 's':
			    sched=optarg;
				break;
			case '?':
			default:
				usage();
	}
	argc -= optind;
	if (argc != 0)
		usage();
	if (isdir==1){
		if(chdir(cdir)<0)
			perror("Can not find the dirctory!");
		else{
			char cpath[128];
			getcwd(cpath,128);
			printf("Current directory:%s\n",cpath);
		    //change the dir
		}
	}
	if(isdebug){
		printf("Dubugging model:\n");
		//create just one thread! 
	}

    if(strcmp(sched,"FCFS")==0)
    	printf("Using FCFS Scheuling!\n");
    else if(strcmp(sched,"SJF")==0)
    	printf("Using SJF Scheuling!\n");
    else
    	printf("Invalid Scheuling!\n");

	socket_fd = socket(AF_INET, SOCK_STREAM,0);    //create the socket
	if(socket_fd<0){
		perror("wrong socket!");
		exit(1);
	}
   
    len=sizeof(client_addr);

	memset((void *)&severv_addr, 0, sizeof(severv_addr));

	severv_addr.sin_family = AF_INET;
	//port = atoi(argv[1]);	
	//severv_addr.sin_port = htons(port);
	if (port == NULL)
		severv_addr.sin_port = htons(0);
	else if (isdigit(*port))
		severv_addr.sin_port = htons(atoi(port));
	else {
		if ((se = getservbyname(port, (char *)NULL)) < (struct servent *) 0) {
			perror(port);
			exit(1);
		}
		severv_addr.sin_port = se->s_port;
	}

	if(bind(socket_fd, (struct sockaddr *) &severv_addr, sizeof(severv_addr)) < 0){
		perror("bind error!");
		exit(1);
	}

	if (getsockname(socket_fd, (struct sockaddr *) &client_addr, &len) < 0) {
		perror("getsockname");
		exit(1);
	}

	fprintf(stderr, "Port number is %d\n", ntohs(client_addr.sin_port));

	pthread_create(&thread_listener,NULL, &Listen_Thread, (void *)socket_fd); // server listener thread!	
	
	//sleep(waitingtime);
	//listen(s,1);
	//pthread_t threads[thread_num];
	//Thread_pool_init(thread_num,flag);
	
	// for(int i=0;o<thread_num;i++){
	// 	cout<<"thread created!"<<endl;
	// 	pthread_create(&threads[i],NULL,request_process, NULL);
	// }
	//new_socketfd=socket_fd;
	pthread_join(thread_listener, NULL);

	return 0;

}

// void
// usage()
// {
// 	fprintf(stderr, "usage: %s -h host -p port\n", progname);
// 	fprintf(stderr, "usage: %s -s [-p port]\n", progname);
// 	exit(1);
// }
