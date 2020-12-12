/*
  Interface routines for the DS18B20

  WARNING: These routines assume you are using Port C, bit 3
  to communicate with the DS18B20.  If you use a different I/O bit,
  the routines below will have to be changed.
*/

#include <avr/io.h>
#include <util/delay.h>

#include "ds18b20.h"

char ds_reset(void);
void ds_write1bit(void);
void ds_write0bit(void);
unsigned char ds_readbit(void);
void ds_writebyte(unsigned char);
unsigned char ds_readbyte(void);

// DS18B20 function commands
#define DS_SKIP_ROM         0xcc
#define DS_CONVERT_T        0x44
#define DS_COPY_SCRATCHPAD  0x48
#define DS_WRITE_SCRATCHPAD 0x4e
#define DS_READ_SCRATCHPAD  0xbe

// We make the 1-Wire bus high by setting port bit for input
// and the external pull-up resistor will make it go high.
// DDR bit = 0, PORT bit = 0
//
// We make the 1-Wire bus low by setting port bit for output,
// and with a zero in the PORT bit it will go low.
// DDR bit = 1, PORT bit = 0

/*
  ds_init - Initialize the port bit used for the 1-Wire bus.
*/
char ds_init(void)
{
    unsigned char i, scratchpad[9];

    // Set the 1-Wire port bit for input with the internal pull-up turned off.
    DDRC &= ~(1 << PC3);        // Put bus in high-Z state
    PORTC &= ~(1 << PC3);       // Put a zero in the port bit
                                // which also turns off internal pullup

    // Set the configuration register for 12-bit conversions
    // just in case some chips got set to lower resolution.
    if (ds_reset()) {           // Reset the DS18B20
	ds_writebyte(DS_SKIP_ROM);    // Send a "Skip ROM" command
	ds_writebyte(DS_READ_SCRATCHPAD); // Send a "Read Scratchpad" command
	for (i = 0; i < 9; i++) {      // Read 9 bytes of scratchpad data
	    scratchpad[i] = ds_readbyte();
	}
    }
    else
	return(0);

    if (ds_reset()) {           // Reset the DS18B20
	ds_writebyte(DS_SKIP_ROM);     // Send a "Skip ROM" command
	ds_writebyte(DS_WRITE_SCRATCHPAD); // Send a "Write Scratchpad"
	ds_writebyte(scratchpad[2]);
	ds_writebyte(scratchpad[3]);
	ds_writebyte(0x7f);    // Set for 12-bit conversions
    }
    else
	return(0);

    _delay_us(100);

    if (ds_reset()) {           // Reset the DS18B20
	ds_writebyte(DS_SKIP_ROM);     // Send a "Skip ROM" command
	ds_writebyte(DS_COPY_SCRATCHPAD);  // Send a "Copy Scratchpad"
    }
    else
	return(0);

    _delay_ms(20);

    return(1);
}

/*
  ds_convert - Send a "CONVERT_T" command to start a conversion.  Don't
  wait for it to finish.
*/
void ds_convert()
{
    if (ds_reset()) {                   // Reset the DS18B20
	ds_writebyte(DS_SKIP_ROM);  // Send a "Skip ROM" command
	ds_writebyte(DS_CONVERT_T); // Send a "Convert T" command
    }
}

/*
  ds_temp - Read a bit from the DS18B20.  Return 0 if the bit is a zero
  (conversion not complete).  If the bit is a one read the 9 bytes in
  the scratchpad and return the first two in an array.
*/
char ds_temp(unsigned char *tdata)
{
    unsigned char i;

    if (ds_readbit()) {                 // Wait for a read to return a one
	if (ds_reset()) {               // Reset the DS18B20
	    ds_writebyte(DS_SKIP_ROM);  // Send a "Skip ROM" command
	    ds_writebyte(DS_READ_SCRATCHPAD);  // Send a "Read Scratchpad" cmd
	    *tdata++ = ds_readbyte();   // Read the first byte (LSB)
	    *tdata = ds_readbyte();     // Read the second byte (MSB)
	    for (i = 0; i < 7; i++) {   // Read the rest of the scratchpad data
		(void) ds_readbyte();
	    }
	}
	return(1);
    }
    else
	return(0);
}

/*
  ds_reset - Send a reset pulse out the bus and then check for
  the "presence" pulse that a device on the bus should send back.
  The presense pulse could start 15usec after the bus goes high
  and be gone by 75usec, or could start in 60usec and be gone by
  120usec.  We'll split the difference between 60 and 75usec and
  wait 67usec.
*/
char ds_reset(void)
{
    DDRC |= (1 << PC3);         // Pull bus low
    _delay_us(600);             // Delay >480usec
    DDRC &= ~(1 << PC3);        // Let bus go high
    _delay_us(67);              // Wait for Presense pulse
    if ((PINC & (1 << PC3)) == 0) {
	while ((PINC & (1 << PC3)) == 0) {} // Wait for pulse to end
	_delay_us(50);          // Probably don't need to do this
        return(1);              // Return status OK
    }
    else
	return(0);              // Return reset failed

}

/*
  ds_writebyte - Write an 8-bit byte out the bus
*/
void ds_writebyte(unsigned char x)
{
    unsigned char i;

    i = 8;
    while (i != 0) {
        if (x & 1)              // Check the LSB
            ds_write1bit();     // Send a one
        else
            ds_write0bit();     // Send a zero
        x >>= 1;                // Shift x over towards the LSB
        i--;
    }
}

/*
  ds_readbyte - Read an 8-bit byte from some device on the bus
*/
unsigned char ds_readbyte()
{
    unsigned char i, x, m;

    x = 0;
    m = 1;                      // m = mask for sticking bits in x
    i = 8;
    while (i != 0) {
        if (ds_readbit())       // Get a bit
            x |= m;             // If a one, put a one in x
        m <<= 1;                // Shift the mask towards the MSB
        i--;
    }
    return(x);
}

/*
  ds_write1bit - Write a single 1 bit out the bus
*/
void ds_write1bit(void)
{

    // Add code to write a 1 to the remote device
    // See page 15 and 16 of DS128B20 datasheet
    DDRC |= (1<<PC3); //pull bus low for 2 us
    _delay_us(2);
    DDRC &= ~(1<<PC3); //let bus go high for 60 us (read time slot)
    _delay_us(60);

}

/*
  ds_write0bit - Write a single 0 bit out the bus
*/
void ds_write0bit(void)
{

    // Add code to write a 0 to the remote device
    // See page 15 and 16 of DS128B20 datasheet

    DDRC |= (1<<PC3); //pull bus low for 60us
    _delay_us(60);
    DDRC &= ~(1<<PC3); //let bus go high
    _delay_us(2);
}

/*
  ds_readbit - Read a single bit from the bus.  Return
  value is either 0 or non-zero.
*/
unsigned char ds_readbit(void)
{

    // Add code to read a bit from the remote device
    // See page 16 and 17 of DS128B20 datasheet
    DDRC |= (1<<PC3); //pull bus low for 2 us
    _delay_us(2);
    DDRC &= ~(1<<PC3); //let bus go high for 10 us
    _delay_us(10); //sample bus after 10us
    unsigned char bit = (PINC&(1<<PC3)); //sample
    _delay_us(50); //for read time slot 62 us
    return bit;

}
