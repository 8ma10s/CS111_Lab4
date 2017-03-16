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
#include <pthread.h>
#include <time.h>
#include "lcd.h"

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

struct rArg{
	int fd;
	int off;
	int send;
	int metric;
	int period;
	int disp;
	FILE *srlog;
	char buf[1024];
};

void *doReceive(void *vStatus);

int main(){

	//socket variables
	FILE *srlog = fopen("lab4_2.log", "w+");
	int fd, err;
	const char *hostname = "r01.cs.ucla.edu";
	const char *port = "16000";
	struct addrinfo hints, *res;
	struct sockaddr_in address;
	lcd_begin();

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
	char toLog[1024];
	char toDisp[1024];
	time_t timer;
	struct tm *t_st;

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
	status.srlog = srlog;

	pthread_t rt;
	pthread_create(&rt, NULL, doReceive, (void*)&status);

	send(fd, "804608241", 10, 0);
	printf("sent: 804608241\n");

 	while(!status.off){

		//if not supposed to send, continue (that is, status == STOP)
		if(!status.send){
			continue;
		}

		//obtain temperature
		temp = mraa_aio_read(tempSensor);
		time(&timer);
		t_st = localtime(&timer);

		//convert temperature
		R = 1023.0/((float)temp)-1.0;
		R *= 100000.0;

		t = 1.0/(log(R/100000.0)/4275+1/298.15)-273.15;
		if(!status.metric){
			t = (t * 1.8) + 32;
			sprintf(toDisp, "%4.1fF", t);
		}
		else{
			sprintf(toDisp, "%4.1fC", t);
		}
		lcd_write(toDisp);
		setZero();

		
		sprintf(toSend, "804608241 TEMP=%4.1f", t);
		sprintf(toLog, "%02d:%02d:%02d %4.1f", t_st->tm_hour, t_st->tm_min, t_st->tm_sec, t);

		//output each second
		if(status.off){
			break;
		}
		if(status.send == 0){
			continue;
		}

		send(fd, toSend, 20, 0);
		printf("sent: %s\n", toSend);

		pthread_mutex_lock(&m);
		fprintf(srlog, "%s\n", toLog);
		pthread_mutex_unlock(&m);
		printf("logged: %s\n", toLog);

		sleep(status.period);
	}

	pthread_join(rt, NULL);
	close(fd);
	fclose(srlog);
	freeaddrinfo(res);
	mraa_aio_close(tempSensor);
	lcd_end();
}	

void *doReceive(void *vStatus){
	//set up variables
	struct rArg *status = (struct rArg*) vStatus;
	int fd = status->fd;
	char *buf = status->buf;
	int i, arg;
	int isValid;
	while(!status->off){
		isValid= 1;
		int bytes = recv(fd, buf, 1024, 0);
		buf[bytes] = '\0';
		printf("received: %s\n", buf);
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
		//scal	e
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
				display();
				status->disp = 1;
			}
			else if(buf[5] == 'N'){
				noDisplay();
				status->disp = 0;
			}
			else{
				sprintf(buf, "%s I", buf);
			}
		}
		else{
			sprintf(buf, "%s I", buf);
		}

		pthread_mutex_lock(&m);
		fprintf(status->srlog, "%s\n", buf);
		pthread_mutex_unlock(&m);
		printf("logged: %s\n", buf);
	}
}
