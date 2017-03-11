#include <stdio.h>
#include <math.h>
#include <mraa/aio.h>

const int B = 4275;
const int R0 = 100000;

int main(){
	int i;
	mraa_aio_context tempSensor;
	tempSensor = mraa_aio_init(0);
	int temp;
	float R, t; 
	for (i = 0; i < 60; i++){
		temp = mraa_aio_read(tempSensor);
		R = 1023.0/((float)temp)-1.0;
		R *= 100000.0;

		t = 1.0/(log(R/100000.0)/B+1/298.15)-273.15;

		printf("temperature is: %f\n", t);
		sleep(1);
	}
}
