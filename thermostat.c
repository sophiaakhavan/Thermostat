/********************************************
 *
 *  Name: Sophia Akhavan
 *  Email: sakhavan@usc.edu
 *  Section: wed 12:30-1:50
 *  Assignment: Project - Thermostat
 *
 ********************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "lcd.h"
#include "ds18b20.h"
#include "encoder.h"
#include "thermostat.h"


//note: 40<=lowthresh<=highthresh, lowthresh<=highthresh<=100

volatile char h_thresh=70;
volatile char l_thresh=70;

//volatile char h_thresh=75; //used as extern in encoder.h - set default to room temp
//volatile char l_thresh=70;

volatile int selecthigh=0;
volatile int selectlow=0;

volatile int old_temp;
volatile int new_temp;

//volatile int count = 0;		// Count to display


int read_temp(unsigned char* t);
char checkInput(char bit);
void AC_heater(int,char,char);

char ds_init(void);
void ds_convert(void);
char ds_temp(unsigned char *);

int main(void) {
    // Initialize DDR and PORT registers and LCD
	lcd_init();
	
	PORTC|=((1<<1)|(1<<2)); //pullup resistors for buttons (low is 1, high is 2)
	DDRD |= ((1<<2)|(1<<3)); //LEDS -- (heater PD2, AC PD3)
	ds_init(); //ds18b20 (thermometer)
	encoder_init(); //encoder



	sei(); //global interrupts

    // Write a spash screen to the LCD
	lcd_writecommand(1);
	lcd_moveto(0,1);
	lcd_stringout("Sophia Akhavan");
	lcd_moveto(1,2);
	lcd_stringout("is super cool");
	_delay_ms(1000);
	lcd_writecommand(1);
	//end

	//read from eeprom for thresholds
	char temp = eeprom_read_byte((void*) 0); //low thresh (updated in encoder.c)
	if(temp>=40 && temp<=100)
	{
		l_thresh=temp;
	}
	temp=eeprom_read_byte((void*) 1); //high thresh
	if(temp>=40 && temp<=100)
	{
		h_thresh=temp;
	}

	//read temperature values
	unsigned char t[2]; //to store two 8-bit values
	if(ds_init()==0) //initialize the DS18B20
	{
		//sensor not responding
	}
	ds_convert(); //start first temp conversion
	

	lcd_moveto(0,0);
	lcd_stringout("Temp: "); //0,6 will be where we put the temp
	lcd_moveto(1,0);
	lcd_stringout("Low= "); //1,4 will be where we replace w/ ?
	//1,5 is where l_thresh goes
	lcd_moveto(1,7);
	lcd_stringout("High= "); //1,12 will be replaced ?
	//1,13 is where h_thresh goes

	char buf[17];

	lcd_moveto(1,4); //initial l_thresh display
	snprintf(buf,17," %d",l_thresh);
	lcd_stringout(buf);

	lcd_moveto(1,12); //initial h_thresh display
	snprintf(buf,17," %d",h_thresh);
	lcd_stringout(buf);


	
    while (1) {
		//keep retrieving new temperature

		if(ds_temp(t)) //true if conversion complete
		{
			//process values returned in t[0] and t[1] to find temperature
			new_temp = read_temp(t); //remember:function returns an int value
			ds_convert(); //start next conversion
		}

		//button presses
		if(checkInput(2)) //button 2 pressed (A2)
		{
			selecthigh=1;
			selectlow=0;
		}
		if(checkInput(1)) //button 1 pressed (A1)
		{
			selecthigh=0;
			selectlow=1;
		}

		if(selectlow)
		{
			lcd_moveto(1,7);
			lcd_stringout("High= ");//to make sure we don't have two ?'s
			lcd_moveto(1,4);
			snprintf(buf,17,"?%d",l_thresh);
			lcd_stringout(buf);
		}
		else if(selecthigh)
		{
			lcd_moveto(1,0);
			lcd_stringout("Low= "); //to make sure we don't have two ?'s
			lcd_moveto(1,12);
			snprintf(buf,17,"?%d",h_thresh);
			lcd_stringout(buf);
			if(h_thresh<100)
			{
				lcd_moveto(1,15);
				lcd_stringout(" ");
			}
		}

		//rest of code depends on if temp has changed

		//display temp on first row
		if(new_temp!=old_temp)
		{
			lcd_moveto(0,6);
			snprintf(buf,17,"%d.%d",new_temp/10,new_temp%10);
			lcd_stringout(buf);
			old_temp=new_temp; //update old temp
		}
		

		AC_heater(new_temp,h_thresh,l_thresh); //turn on/off appropriate LED

		

		_delay_ms(10);
    }
}



int read_temp(unsigned char* t) //convert and return temperature in fahrenheit
{
	double val;
	//read two bits from DS18B20
	double num; //want to combine two 8-bit values into single signed 16-bit variable
	num = ( (t[1]<<8) + t[0] ); //shift by 8 bits for calculation and OR(+) to combine
	val=(num*9/(5*16) + 32); //convert to fahrenheit (remember scale 16)
	return (int)(val*10);

}

//check if button is pressed
char checkInput(char bit)
{
  //if PINC at bit is 0 (voltage 0) (button pressed, LED on)
  if((PINC & (1<<bit))==0){ 
    return 1;
  }
  else {
    return 0;
  }
}

//turn on/off appropriate LED
void AC_heater(int new_temp,char h_thresh,char l_thresh)
{
	if((new_temp/10)+1>h_thresh)
	{
		PORTD|=(1<<3); //AC on
	}
	else
	{
		PORTD&=~(1<<3); //make sure AC off
	}
	if((new_temp/10)<l_thresh)
	{
		PORTD|=(1<<2); //heater on
	}
	else
	{
		PORTD&=~(1<<2); //make sure heater off
	}

}


