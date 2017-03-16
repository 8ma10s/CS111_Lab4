#include <stdio.h>
#include <inttypes.h>
#include <mraa/i2c.h>
#include "lcd.h"

void lcd_begin(){
	lcd = mraa_i2c_init(0);

	_displayfunction |= LCD_2LINE;
	_numlines = 2;
	_currline = 0;

	sleep(1);

	command(LCD_FUNCTIONSET | _displayfunction);
	sleep(1);
	command(LCD_FUNCTIONSET | _displayfunction);

	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();

	clear();

	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	command(LCD_ENTRYMODESET | _displaymode);

	setReg(REG_MODE1, 0);
	setReg(REG_OUTPUT, 0xFF);
	setReg(REG_MODE2, 0x20);
	
	setReg(REG_RED, (unsigned char) 255);
	setReg(REG_GREEN, (unsigned char) 255);
	setReg(REG_BLUE, (unsigned char) 255);
}

void i2c_send_bytes(unsigned char *dta, unsigned char len){
	mraa_i2c_address(lcd, LCD_ADDRESS);
	int i;
	mraa_i2c_write(lcd, (uint8_t*) dta, (int) len);
}

void command(uint8_t value){
	unsigned char dta[2] = {0x80, value};
	i2c_send_bytes(dta, 2);
}

void display(){
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);

}

void noDisplay(){
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);

}

void clear(){
	command(LCD_CLEARDISPLAY);

}

void setReg(unsigned char addr, unsigned char dta){
	mraa_i2c_address(lcd, RGB_ADDRESS);
	unsigned char toSend[2] = {addr, dta};
	mraa_i2c_write(lcd, (uint8_t*)toSend, 2);
}

void lcd_end(){
	noDisplay();
	setReg(REG_RED, (unsigned char) 0);
	setReg(REG_GREEN, (unsigned char) 0);
	setReg(REG_BLUE, (unsigned char) 0);
	mraa_i2c_stop(lcd);
}

size_t lcd_write(char* str){
	int i;
	unsigned char dta[2] = {0x40, 0};
	for (i = 0; str[i] != '\0'; i++){
		dta[1] = (unsigned char)str[i];
		i2c_send_bytes(dta, 2);
	}
	return (size_t)i;
}

void setZero(){
	unsigned char dta[2] = {0x80, 0x80};
	i2c_send_bytes(dta, 2);

}
