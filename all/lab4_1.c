#include <stdio.h>
#include <math.h>
#include <mraa/aio.h>
#include <time.h>


int main(){
	int i;
	mraa_aio_context tempSensor;
	tempSensor = mraa_aio_init(0);
	FILE *fd = fopen("lab4_1.log", "w+");
	//variables
	int temp;
	float R, t;
	time_t timer;
	struct tm *t_st;
	char out[1024];
	for (i = 0; i < 90; i++){
		//obtain temperature
		temp = mraa_aio_read(tempSensor);
		//obtain time
		time(&timer);

		//convert temperature
		R = 1023.0/((float)temp)-1.0;
		R *= 100000.0;

		t = 1.0/(log(R/100000.0)/4275+1/298.15)-273.15;
		t = (t * 1.8) + 32;
		//convert time
		t_st = localtime(&timer);

		//output each second
		sprintf(out ,"%02d:%02d:%02d %4.1f\n", t_st->tm_hour, t_st->tm_min, t_st->tm_sec,t);
		printf("%s", out);
		fprintf(fd, "%s", out);
		sleep(1);
	}
	fclose(fd);
	mraa_aio_close(tempSensor);
}
