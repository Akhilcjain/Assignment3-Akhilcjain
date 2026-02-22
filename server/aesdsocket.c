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

char *Data = NULL;
int socketfd, connectionfd;
FILE *file;
struct addrinfo *res;
volatile sig_atomic_t exit_requested = 0;

void handle_signal(int signal)
{
	printf("Closing all sockets\n");
	exit_requested = 1;
	
	//shutdown(socketfd, SHUT_RDWR);
}

void cleanup()
{
    if (Data) {
        free(Data);
        Data = NULL;
    }

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

int main(int argc, char* argv[])
{
	// address to bind to 
	//struct sockaddr addr;
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
	int rundeamon = 0;
	if (argc > 1 && strcmp(argv[1], "-d") == 0)
	{
		//printf("Running in deamon mode\n");
    		rundeamon = 1;
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
			if (rundeamon == 1) 
			{
    				int pid = fork();
    				if (pid < 0) {
			        perror("fork");
        			cleanup();
        			return -1;
    				}
    				if (pid > 0) 
				{
        				// Parent exits immediately
        				printf("Daemon parent exiting\n");
       					return 0;
    				}
    				// Child continues as daemon
			}

			printf("Listening to socket\n");
			if(0 != listen(socketfd, 5))
			{
				printf("Listen error\n");
			}
			else
			{
				struct sockaddr AcceptedAddr;
				socklen_t AddrSize = sizeof(AcceptedAddr);
				
				while(!exit_requested)
				{
					connectionfd = accept(socketfd, &AcceptedAddr, &AddrSize);
					
					if(AddrSize == 0)
					{
						printf("Accept command failed\n");
						printf("recv error: %s\n", strerror(errno));
						return -1;
					}
					printf("Connection accepted\n");
					// Time to receive packets
					
					char temp[1024];
					int totalsize = 0;
					int prevsize=0;
					int filesize = 0;
					while (!exit_requested)
					{
				
						int received = recv(connectionfd, temp, 1024, 0);
						if(received == 0)
						{
							close(connectionfd);
							printf("Connection broken\n");
							break;
						}
						printf("%d bytes received\n",received);	
						// Data packet complete
                	              		if(received < 0)
						{
							//printf("recv error: %s\n", strerror(errno));
							//handle_signal(0);
							break;
						}
						if(received > 0)		
						{
							//printf("Packet received %d\n",received);
							totalsize += received;
				
							if(Data == NULL)
							{
							//	printf("Allocating size\n");
								Data = (char *)malloc(totalsize);
							}
							else
							{
							//	printf("Reallocatng. Prev size: %d, new size %d",prevsize, totalsize);
								Data = (char*)realloc(Data, totalsize);
							}
							//printf("Total Bytes %d\n",totalsize);
							if(Data == NULL)
							{
								printf("Data is NULL\n");
								handle_signal(0);
								// Malloc Error
								exit_requested = 1;
								break;
							}
							memcpy(&Data[prevsize],temp, received);
							prevsize = totalsize;
							if(Data[totalsize-1] == '\n')
							{
							//	printf("Packet complete\n");
										// Packet complete, write to data
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
								totalsize = 0;
								prevsize=0;
								free(Data);
								Data = NULL;
								close(connectionfd);
								connectionfd = -1;
							}
						}
					}
				}
			}
		}
	}
	cleanup();
	return 0;
	//handle_signal(0);
}
