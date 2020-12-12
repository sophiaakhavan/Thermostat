#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "encoder.h"
#include "ds18b20.h"
#include "thermostat.h"

volatile char h_thresh;
volatile char l_thresh;
volatile int selecthigh; //set to 1 by button if selected
volatile int selectlow; //set to 1 by button if selected

void encoder_init()
{
    PORTB|=((1<<4)|(1<<3)); //pullup resistors for PB4 and PB3 (encoder inputs)
    PCICR|=(1<<PCIE0); //enable pin change interrupts on port b
    PCMSK0|=((1<<PCINT3)|(1<<PCINT4)); //pin change interrupt register: port b bits 3,4

 
     // Read the A and B inputs to determine the initial state
	char input = PINB; //read pinb first in order to read both at same time
	a=(input&(1<<3));
	b=(input&(1<<4));

    if (!b && !a)
	old_state = 0;
    else if (!b && a)
	old_state = 1;
    else if (b && !a)
	old_state = 2;
    else
	old_state = 3;

    new_state = old_state;
}

//isr interrupt service routine
ISR(PCINT0_vect){ //pcint0 register for port b
	char input = PINB; //read inputs at same time
	a=(input&(1<<3));
	b=(input&(1<<4));

	// For each state, examine the two input bits to see if state
	// has changed, and if so set "new_state" to the new state,
	// and adjust the count value.

    //changes to lab 7: now we need to account for max and min temp thresholds
    //instead of a single count we have two for either low or high thresholds
    //new_state = old_state;

	if (old_state == 0) { //00

	    // Handle A and B inputs for state 0
		if(a && !b){ //clockwise

			new_state=1; //go to 01
			//clockwise

            if(l_thresh<h_thresh && selectlow==1) //low selected and under h_thresh
            {
                l_thresh++;
                eeprom_update_byte((void*)0,l_thresh); //update low threshold into address 0 of eeprom
            }

            else if(h_thresh<100 && selecthigh==1) //high selected and under 100
            {
                h_thresh++;
                eeprom_update_byte((void*)1,h_thresh); //update high threshold into address 1 of eeprom
            }
            
		}

		else if(b && !a){ //counter clockwise

			new_state=3; //go to 11
			//ccw

            if(l_thresh>40 && selectlow==1) //low selected and above min(40)
            {
                l_thresh--;
                eeprom_update_byte((void*)0,l_thresh); //update low threshold into address 0 of eeprom
            }

            else if(h_thresh>l_thresh && selecthigh==1) //high selected and above low thresh
            {
                h_thresh--;
                eeprom_update_byte((void*)1,h_thresh); //update high threshold into address 0 of eeprom
            }

		}

		else new_state=0; //dont move

	}

	else if (old_state == 1) { //01

	    // Handle A and B inputs for state 1

		if(b && a){ //clockwise
			new_state=2;
			//cw
            if(l_thresh<h_thresh && selectlow==1) //low selected and under h_thresh
            {
                l_thresh++;
                eeprom_update_byte((void*)0,l_thresh); //update low threshold into address 0 of eeprom
            }

            else if(h_thresh<100 && selecthigh==1) //high selected and under 100
            {
                h_thresh++;
                eeprom_update_byte((void*)1,h_thresh); //update high threshold into address 1 of eeprom
            }

		}

		else if(!a && !b){ //ccw
			new_state=0;
			//ccw
            if(l_thresh>40 && selectlow==1) //low selected and above min(40)
            {
                l_thresh--;
                eeprom_update_byte((void*)0,l_thresh); //update low threshold into address 0 of eeprom
            }

            else if(h_thresh>l_thresh && selecthigh==1) //high selected and above low thresh
            {
                h_thresh--;
                eeprom_update_byte((void*)1,h_thresh); //update high threshold into address 0 of eeprom
            }

		}

		else new_state=1;

	}

	else if (old_state == 2) {

	    // Handle A and B inputs for state 2
		if(!a && b){
			new_state=3;
			//cw
            if(l_thresh<h_thresh && selectlow==1) //low selected and under h_thresh
            {
                l_thresh++;
                eeprom_update_byte((void*)0,l_thresh); //update low threshold into address 0 of eeprom
            }

            else if(h_thresh<100 && selecthigh==1) //high selected and under 100
            {
                h_thresh++;
                eeprom_update_byte((void*)1,h_thresh); //update high threshold into address 1 of eeprom
            }
		}
		else if(a && !b){
			new_state=1;
			
            if(l_thresh>40 && selectlow==1) //low selected and above min(40)
            {
                l_thresh--;
                eeprom_update_byte((void*)0,l_thresh); //update low threshold into address 0 of eeprom
            }

            else if(h_thresh>l_thresh && selecthigh==1) //high selected and above low thresh
            {
                h_thresh--;
                eeprom_update_byte((void*)1,h_thresh); //update high threshold into address 0 of eeprom
            }
		}
		else new_state=2;
	}
    //3
	else {

		if(!b && !a){
			new_state=0;
			//cw
            if(l_thresh<h_thresh && selectlow==1) //low selected and under h_thresh
            {
                l_thresh++;
                eeprom_update_byte((void*)0,l_thresh); //update low threshold into address 0 of eeprom
            }

            else if(h_thresh<100 && selecthigh==1) //high selected and under 100
            {
                h_thresh++;
                eeprom_update_byte((void*)1,h_thresh); //update high threshold into address 1 of eeprom
            }
		}
		else if(a && b){
			new_state=2;
			
            if(l_thresh>40 && selectlow==1) //low selected and above min(40)
            {
                l_thresh--;
                eeprom_update_byte((void*)0,l_thresh); //update low threshold into address 0 of eeprom
            }

            else if(h_thresh>l_thresh && selecthigh==1) //high selected and above low thresh
            {
                h_thresh--;
                eeprom_update_byte((void*)1,h_thresh); //update high threshold into address 0 of eeprom
            }
		}
		else new_state=3;
	}

	// If state changed, update the value of old_state
	if (new_state != old_state) {
	    old_state = new_state;
	}
}