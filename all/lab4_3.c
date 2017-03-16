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

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>


pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

struct rArg{
	SSL  *ssl;
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

	char dest_url[] = "r01.cs.ucla.edu";
	BIO *certbio = NULL;
	BIO *outbio = NULL;
	X509 *cert = NULL;
	X509_NAME *certname = NULL;
	const SSL_METHOD *method;
	SSL_CTX *ctx;
	SSL *ssl;
	int server = 0;
	int ret, i;

	//initialize openssl
	OpenSSL_add_all_algorithms();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();
	SSL_load_error_strings();

	//create input/output BIO
	certbio = BIO_new(BIO_s_file());
	outbio  = BIO_new_fp(stdout, BIO_NOCLOSE);

	//initialize SSL library and register alg
	if(SSL_library_init() < 0)
		BIO_printf(outbio, "Could not initialize the OpenSSL library !\n");
	//setSSLv2
	method = SSLv23_client_method();

	//create new SSL context
	if((ctx = SSL_CTX_new(method)) == NULL)
		BIO_printf(outbio, "Unable to create a new SSL context structure.\n");
	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);
	//create new SSL connection state obj
	ssl = SSL_new(ctx);

	//socket variables
	FILE *srlog = fopen("lab4_3.log", "w+");
	int fd, err;
	const char *hostname = "r01.cs.ucla.edu";
	const char *port = "17000";
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

	SSL_set_fd(ssl, fd);

	if ( SSL_connect(ssl) != 1 )
		BIO_printf(outbio, "Error: Could not build a SSL session to: %s.\n", dest_url);
	else
		BIO_printf(outbio, "Successfully enabled SSL/TLS session to: %s.\n", dest_url);


	//get remote certificate
	cert = SSL_get_peer_certificate(ssl);
	if (cert == NULL)
		BIO_printf(outbio, "Error: Could not get a certificate from: %s.\n", dest_url);
	else
		BIO_printf(outbio, "Retrieved the server's certificate from: %s.\n", dest_url);


	//extract various certificat info
	certname = X509_NAME_new();
	certname = X509_get_subject_name(cert);

	//display cert subject
	BIO_printf(outbio, "Displaying the certificate subject data:\n");
	X509_NAME_print_ex(outbio, certname, 0, 0);
	BIO_printf(outbio, "\n");



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

	//initialize struct
	status.ssl = ssl;
	status.off = 0;
	status.send = 1;
	status.metric = 0;
	status.period = 3;
	status.disp = 1;
	status.srlog = srlog;

	pthread_t rt;
	pthread_create(&rt, NULL, doReceive, (void*)&status);

	SSL_write(ssl, "804608241", 10);
	printf("804608241\n");

 	while(!status.off){

		//if not supposed to send, continue (that is, status == STOP)
		if(!status.send){
			continue;
		}

		//obtain temperature
		temp = mraa_aio_read(tempSensor);

		//obtain time
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
		
		SSL_write(ssl, toSend, 20);
		printf("sent: %s\n", toSend);

		pthread_mutex_lock(&m);
		fprintf(srlog, "%s\n", toLog);
		pthread_mutex_unlock(&m);
		printf("logged: %s\n", toLog);

		sleep(status.period);
	}


	//free structures
	pthread_join(rt, NULL);
	SSL_free(ssl);
	X509_free(cert);
	SSL_CTX_free(ctx);
	BIO_printf(outbio, "Finished SSL/TLS connection with server: %s.\n", "r01.cs.ucla.edu");
	close(fd);
	fclose(srlog);
	freeaddrinfo(res);
	mraa_aio_close(tempSensor);
  lcd_end();
}	

void *doReceive(void *vStatus){
	//set up variables
	struct rArg *status = (struct rArg*) vStatus;
	SSL *ssl = status->ssl;
	char *buf = status->buf;
	int i, arg;
	int isValid;

	while(!status->off){
		isValid= 1;
		int bytes = SSL_read(ssl, buf, 1024);
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
