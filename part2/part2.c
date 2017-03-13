#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <math.h>
#include <mraa/aio.h>
#include <string.h>
#include <ctype.h>

struct rArg{
	int fd;
	int off;
	int send;
	int metric;
	int period;
	int disp;
	char buf[1024];
};

void doReceive(struct rArg *status);

int main(){

	//socket variables
	int fd, err;
	const char *hostname = "r01.cs.ucla.edu";
	const char *port = "16000";
	struct addrinfo hints, *res;
	struct sockaddr_in address;

	//setup
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//get address info
	if((err = getaddrinfo(hostname, port, &hints, &res)) != 0){
		fprintf(stderr, "Failed to get address info: error code %d\n", err);
		exit(1);
	}

	//connect
	fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	connect(fd, res->ai_addr, res->ai_addrlen);

	//initialize sensor
	mraa_aio_context tempSensor;
	tempSensor = mraa_aio_init(0);
	//variables for sending
	int temp;
	float R, t;
	char toSend[] = "804608241 TEMP=##.#";

	//variables for receiving
	struct rArg status;
	fd_set fds;
	struct timeval timeout;

	//initialize struct
	status.fd = fd;
	status.off = 0;
	status.send = 1;
	status.metric = 0;
	status.period = 3;
	status.disp = 1;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	send(fd, "804608241", 10, 0);
 	while(!status.off){

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		select(fd+1, &fds, NULL, NULL, &timeout);

		if(FD_ISSET(fd, &fds)){
			doReceive(&status);
		}

		//if not supposed to send, continue (that is, status == STOP)
		if(!status.send){
			continue;
		}

		//obtain temperature
		temp = mraa_aio_read(tempSensor);

		//convert temperature
		R = 1023.0/((float)temp)-1.0;
		R *= 100000.0;

		t = 1.0/(log(R/100000.0)/4275+1/298.15)-273.15;
		if(!status.metric){
			t = (t * 1.8) + 32;
		}
		
		sprintf(toSend, "804608241 TEMP=%4.1f", t);

		//output each second
		send(fd, toSend, 20, 0);
		printf("%s\n", toSend);
		sleep(status.period);
	}

	close(fd);
	freeaddrinfo(res);
	mraa_aio_close(tempSensor);
}	

void doReceive(struct rArg *status){
	//set up variables
	int fd = status->fd;
	char *buf = status->buf;
	int i, arg;
	int isValid = 1;
	int bytes = recv(fd, buf, 1024, 0);
	buf[bytes] = '\0';
	printf("received command: %s, num: %d\n", buf, bytes);
	//off
	if(strcmp(buf, "OFF") == 0){
		status->off = 1;
	}
	//start
	else if(strcmp(buf, "START") == 0){
		status->send = 1;
	}
	//stop
	else if(strcmp(buf, "STOP") == 0){
		status->send = 0;
	}
	//period
	else if(strncmp(buf, "PERIOD=", 7) == 0){
		for (i = 7; buf[i] != '\0'; i++){
			if(!isdigit((int)buf[i])){
				sprintf(buf, "%s I", buf);
				isValid = 0;
				break;
			}
		}
		if(isValid){
			arg = atoi(buf+7);
			if(arg < 1 || arg > 3600){
				sprintf(buf, "%s I", buf);
			}
			else{
				status->period = arg;
			}
		}
	}
	//scale
	else if(strncmp(buf, "SCALE=", 6) == 0){
		if(strlen(buf) != 7){
			sprintf(buf, "%s I", buf);
		}
		else if(buf[6] == 'F'){
			status->metric = 0;
		}
		else if(buf[6] == 'C'){
			status->metric = 1;
		}
		else{
			sprintf(buf, "%s I", buf);
		}
	}
	//disp
	else if(strncmp(buf, "DISP ", 5) == 0){
		if(strlen(buf) != 6){
			sprintf(buf, "%s I", buf);
		}
		else if (buf[5] == 'Y'){
			status->disp = 1;
		}
		else if(buf[5] == 'N'){
			status->disp = 0;
		}
		else{
			sprintf(buf, "%s I", buf);
		}
	}
	else{
		sprintf(buf, "%s I", buf);
	}
	
	printf("%s\n", buf);
}
