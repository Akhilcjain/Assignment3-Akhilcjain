#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/queue.h>
#include <pthread.h>
#include <stdbool.h>


int socketfd, connectionfd;
FILE *file;
struct addrinfo *res;
volatile sig_atomic_t exit_requested = 0;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

struct ThreadNode_st{
	pthread_t 	ThreadId;
	pthread_mutex_t *file_mutex;
	int		ConnectionId;
	bool		ThreadComplete;
	SLIST_ENTRY(ThreadNode_st) entries;
};

// Define head type
SLIST_HEAD(ThreadList_st, ThreadNode_st);

void handle_signal(int signal)
{
	printf("Closing all sockets\n");
	exit_requested = 1;
	
	//shutdown(socketfd, SHUT_RDWR);
}

void cleanup()
{

    if (connectionfd != -1) {
        close(connectionfd);
        connectionfd = -1;
    }

    if (socketfd != -1) {
        close(socketfd);
        socketfd = -1;
    }

    if (file) {
        fclose(file);
        file = NULL;
    }

    if (res) {
        freeaddrinfo(res);
        res = NULL;
    }
}

void* handle_connection(void * args)
{

	struct ThreadNode_st * NodeInfo = (struct ThreadNode_st *)args;
	char *Data = NULL;
        char temp[1024];
        int totalsize = 0;
        int prevsize=0;
        int filesize = 0;
	int connectionfd = NodeInfo->ConnectionId;

        while (!exit_requested)
     	{
		int received = recv(connectionfd, temp, 1024, 0);
                if(received == 0)
                {
                	printf("Connection broken\n");
                        break;
                }
                printf("%d bytes received\n",received);       
                                                
                if(received < 0)
                {

                	printf("recv error: %s\n", strerror(errno));                             
			
			break;
		}
                if(received > 0)                                
		{
                	printf("Packet received %d\n",received);
                        totalsize += received;

                        Data = (Data == NULL)? (char *)malloc(totalsize):(char*)realloc(Data, totalsize);
                        memcpy(&Data[prevsize],temp, received);
                        prevsize = totalsize;
                        if(Data[totalsize-1] == '\n')
                        {
				pthread_mutex_lock(NodeInfo->file_mutex);
                        	fwrite(Data, totalsize,1, file);
                                filesize += totalsize-1;
                                fseek(file, 0, SEEK_SET);
                                // Write back data
                                char tempdata[1024];
                                int bytesread = 1;

                                 bytesread = fread(tempdata,1, sizeof(tempdata), file);
                                 while(bytesread != 0)
                                 {
                                 	send(connectionfd, tempdata, bytesread,0);
                                        bytesread = fread(tempdata, 1, sizeof(tempdata),file);
                                 }
                                 fseek(file, 0, SEEK_END);
				 pthread_mutex_unlock(NodeInfo->file_mutex);
                                 totalsize = 0;
                                 prevsize=0;
                         }                                
		}  
	}
	printf("Thread complete\n");
	free(Data);
	NodeInfo->ThreadComplete = true;
	return NULL;
}

void *TimeStampFunc(void *args)
{
	struct ThreadNode_st * NodeInfo = (struct ThreadNode_st *)args;
	while(!exit_requested)
	{
		char time_str[100];
		time_t now;
		struct tm *tmp;
		sleep(10);
		time(&now);
		tmp = localtime(&now);

		if(strftime(time_str, sizeof(time_str),"timestamp:%a, %d %b %Y %H:%M:%S %z\n", tmp) == 0) {
			fprintf(stderr, "strftime returned 0");
			continue;
		}
		pthread_mutex_lock(NodeInfo->file_mutex);
		fwrite(time_str, strlen(time_str),1, file);
		pthread_mutex_unlock(NodeInfo->file_mutex);
		//sleep(10);
	}
	NodeInfo->ThreadComplete = true;
	return NULL;
}
int main(int argc, char* argv[])
{
	// address to bind to 
	//struct sockaddr addr;
	//Initialize list head
	struct ThreadList_st ThreadList;
	SLIST_INIT(&ThreadList);

	struct addrinfo hints;
	printf("Starting app\n");
	struct sigaction sa;
	sa.sa_handler = handle_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0; // do NOT set SA_RESTART
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	// Open a socket
	socketfd = socket(PF_INET,SOCK_STREAM, 0);
	// Open file
	file = fopen("/var/tmp/aesdsocketdata", "w+");
	//int rundeamon = 0;
	if (argc > 1 && strcmp(argv[1], "-d") == 0)
	{
		printf("Running in deamon mode\n");
    		//rundeamon = 1;
	}

	if(socketfd == -1)
	{
		// Log Error Code
	}
	else if(NULL == file)
	{
		// Log data
	}
	else
	{	
		printf("reached else\n");
		int opt =1;
		setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		if(0 != getaddrinfo(NULL, "9000", &hints, &res))
		{
			printf("getaddr fail\n");
		}	
		else if(0 != bind(socketfd, res->ai_addr, res->ai_addrlen))
		{
			// binding failed, report error
			printf("bind fail \n");
		}
		else
		{
			

			printf("Listening to socket\n");
			if(0 != listen(socketfd, 5))
			{
				printf("Listen error\n");
			}
			else
			{
				// Start a new thread to write timestamps
				struct ThreadNode_st * TimeStampnode = (struct ThreadNode_st *)malloc(sizeof(struct ThreadNode_st));
				TimeStampnode->ConnectionId = 0;
				TimeStampnode->ThreadComplete = false;
                                TimeStampnode->file_mutex = &file_mutex;
				if(pthread_create(&TimeStampnode->ThreadId, NULL, TimeStampFunc, TimeStampnode))
				{
					printf("Error Creating TimeStampNode\n");

				}
				SLIST_INSERT_HEAD(&ThreadList, TimeStampnode, entries);

				printf("timeStamp Node created\n");
				struct sockaddr AcceptedAddr;
				socklen_t AddrSize = sizeof(AcceptedAddr);
				
				while(1)
				{
					if(!exit_requested)
					{
						connectionfd = accept(socketfd, &AcceptedAddr, &AddrSize);
					
					
						// Spawn a thread
						// Create a new node
						struct ThreadNode_st * node = (struct ThreadNode_st *)malloc(sizeof(struct ThreadNode_st));
						node->ConnectionId = connectionfd;
						node->file_mutex = &file_mutex;
						node->ThreadComplete = false;
						if(0 != pthread_create(&node->ThreadId, NULL, handle_connection, node))
						{
							printf("Error in creating thread\n");
						}
						// Add node in list
						SLIST_INSERT_HEAD(&ThreadList, node, entries);
					}

					struct ThreadNode_st *cursor;
			//		struct ThreadNode_st tmp;
					cursor = SLIST_FIRST(&ThreadList);
					struct ThreadNode_st *next;

					while(cursor != NULL)
					{
    						next = SLIST_NEXT(cursor, entries);

    						if(cursor->ThreadComplete)
    						{
        						pthread_join(cursor->ThreadId, NULL);
        						SLIST_REMOVE(&ThreadList, cursor, ThreadNode_st, entries);
        						free(cursor);
    						}

    						cursor = next;
					}


					if(exit_requested)
					{
						break;
					}
				}
			}
		}
	}
	cleanup();
	return 0;
}
