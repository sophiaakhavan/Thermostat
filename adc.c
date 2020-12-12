#include <avr/io.h>

#include "adc.h"

#define MASKBITS 0x0f

void adc_init(void)
{
    // Initialize the ADC
    ADMUX &= ~(1<<REFS1); //01 to REFS to select AVCC
    ADMUX |= (1<<REFS0);
    ADMUX |= (1<<ADLAR); //8-bit conversion value
    //we want clock in range 50-200khz
    //since arduino processor is 16mhz, dividing by 128 gives us 125 khz
    ADCSRA |= ((1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2)); //set prescalar to 128
    ADCSRA |= (1<<ADEN); //enable the ADC module

}

unsigned char adc_sample(unsigned char channel)
{
    ADMUX &= ~MASKBITS; //clear bits
    // Set ADC input mux bits to 'channel' value
    ADMUX |= (channel&MASKBITS);
    // Convert an analog input and return the 8-bit result
    ADCSRA |= (1<<ADSC); //start conversion process
    while((ADCSRA&(1<<ADSC))!=0){ //test adsc
    }
    unsigned char result = ADCH;
    return result;
}
