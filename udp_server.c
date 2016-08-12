#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include "qemu_command.h"

#define BUFLEN 512
#define NPACK 10
#define PORT 5055 

int s = -1;
int trace_fd = -1, trace_clear_fd = -1;
static volatile int isTerminated = 0;

void intHandler(int dummy){
	isTerminated = 1;
}

void diep(char *string)
{
	perror(string);
	close(s);
	close(trace_fd);
	close(trace_clear_fd);
	exit(1);
}

void processCmd(int cmd)
{
        if(cmd == CMD_TRACE_OFF){ 
                if(write(trace_fd, "0", 1) <= 0)
                        perror("WRITE_FD 0 FAILED");
        }
        else if (cmd == CMD_TRACE_ON){
                if(write(trace_fd, "1", 1) <= 0)
                        perror("WRITE_FD 1 FAILED");
        }
        else if (cmd == CMD_TRACE_CLEAR){
                if(write(trace_clear_fd, "0", 1) <= 0)
                        perror("TRACE_CLEAR_FD 0 FAILED");
        }
        else if (cmd == CMD_SERVER_CLOSE){
                diep("SERVER_CLOSE_FROM_HOST");
        }

}

int main(void)
{
	const char *debugfs;
	char path[256];
	struct sockaddr_in si_me, si_other;
	tcmd cmd = {.cmdNo = 0};
	int i, slen=sizeof(si_other);
	char buf[BUFLEN];

	signal(SIGINT, intHandler);

	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
		diep("socket");

	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (struct sockaddr*)&si_me, sizeof(si_me))==-1)
		diep("bind");

	debugfs = find_debugfs();
	if(debugfs) {
		printf("DEBUGFS: %s\n", debugfs);
		strcpy(path, debugfs);
		strcat(path, "/tracing/tracing_on");
		printf("TRACING_ON: %s\n", path);
		trace_fd = open (path, O_WRONLY);
		strcpy(path, debugfs);
		strcat(path, "/tracing/trace");
		printf("TRACING_ON: %s\n", path);
		if (trace_fd < 0) {
			diep("TRACING_ON FD");		
		}
		trace_clear_fd = open (path, O_WRONLY);
		if (trace_clear_fd < 0) {
			diep("TRACING_CLEAR FD");
		}

		while(!isTerminated) {
			if (recvfrom(s, &cmd, sizeof(tcmd), 0, (struct sockaddr*)&si_other, &slen)==-1)
				diep("recvfrom()");
			printf("Received packet from %s:%d\nCommand: 0x%08x\n\n", 
					inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), cmd.cmdNo);

			processCmd(cmd.cmdNo);
		}
	}	
	close(s);
	close(trace_fd);
	close(trace_clear_fd);
	return 0;
}
