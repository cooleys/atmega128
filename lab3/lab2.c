// lab2.c
// Sarah Cooley
// ECE473 Lab2 - Fall2010

//	HARDWARE SETUP:
//	PORTA is connected to the segments of the LED display. and to the pushbuttons.
//	PORTA.0 corresponds to segment a, PORTA.1 corresponds to segement b, etc.
//	PORTB bits 4-6 go to a,b,c inputs of the 74HC138.
//	PORTB bit 7 goes to the PWM transistor base.

#define F_CPU 16000000 // cpu speed in hertz
#define TRUE 1
#define FALSE 0
#include <avr/io.h>
#include <util/delay.h>

//holds data to be sent to the segments. logic zero turns segment on
uint8_t segment_data[5]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//decimal to 7-segment LED display encodings, logic "0" turns on segment
uint8_t dec_to_7seg[12] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80,
	0x90, 0xFC, 0xFF};


//******************************************************************************
//	                          chk_buttons
//Checks the state of all of the buttons. It shifts in ones till a button is
//pushed. Function returns a 1 only once per debounced button push so a debounce
//and toggle function can be implemented at the same time.
//Adapted to check all buttons from Ganssel's "Guide to Debouncing"
//Expects active low pushbuttons on PINA port.	Debounce time is determined by
//external loop delay times 12.
//
//Input:  current button state.
//Return: 8 bit integer with toggled buttons reprisented as 1s.  1s are pressed

uint8_t chk_buttons(uint8_t cstate) {
	static uint16_t state[8] = {0,0,0,0,0,0,0,0}; //holds present state
	uint8_t nstate = 0;
	int i=0;

	for(i=0; i<8; i++)
		state[i] = (state[i] << 1) | (! bit_is_clear(PINA, i));

	for(i=0; i<8; i++)
		nstate |= (state[i] == 0x8000) << i;

	return nstate;
}
//******************************************************************************


//******************************************************************************
//	                                 segment_sum
//takes a 16-bit binary input value and places the appropriate equivalent 4 digit
//BCD segment code in the array segment_data for display.
//array is loaded at exit as:	|digit3|digit2|colon|digit1|digit0|
void segsum(uint16_t sum) {
	segment_data[0] = sum%10;
	sum /= 10;
	segment_data[1] = sum%10;
	sum /= 10;
	segment_data[2] = 11;
	segment_data[3] = sum%10;
	sum /= 10;
	segment_data[4] = sum%10;
	sum /= 10;

	if(segment_data[4]==0){
		segment_data[4] = 11;
		if(segment_data[3]==0){
			segment_data[3] = 11;
			if(segment_data[1]==0){
				segment_data[1] = 11;
				if(segment_data[0]==0)
					segment_data[0] = 0;
			}
		}
	}
}//segment_sum
//******************************************************************************


//******************************************************************************
uint8_t main()
{
uint8_t i=0;
uint16_t count=0;
uint8_t state = 0;

DDRB = 0xF0; //set port bits 4-7 B as outputs

while(1){
	_delay_ms(2); //insert loop delay for debounce

	//make PORTA an input port with pullups
	DDRA = 0x00;
	PORTA = 0xFF;

	//enable tristate buffer for pushbutton switches
	PORTB = 0x70;

	//now check each button and increment the count as needed
	state=chk_buttons(state);

	for(i=0; i<8; i++){
		if(state & 1<<i){
			count += (1<<i);
			//bound the count to 0 - 1023
			if(count > 1023)
				count=1;
		}
	}

	//break up the disp_value to 4, BCD digits in the array: call (segsum)
	segsum(count);

	//make PORTA an output
	DDRA = 0xFF;
	PORTB = 0x00; //digit zero  on
	PORTA = dec_to_7seg[segment_data[0]];
	_delay_ms(2);
	PORTB = 0x10; //digit one   on
	PORTA = dec_to_7seg[segment_data[1]];
	_delay_ms(2);
	PORTB = 0x20; //colon, indicator leds  on
	PORTA = dec_to_7seg[segment_data[2]];
	_delay_ms(2);
	PORTB = 0x30; //digit two   on
	PORTA = dec_to_7seg[segment_data[3]];
	_delay_ms(2);
	PORTB = 0x40; //digit three on
	PORTA = dec_to_7seg[segment_data[4]];
/*
	int x=0;
	//bound a counter (0-4) to keep track of digit to display
	for(i=0x00; i<5; i += 0x10){
		//send 7 segment code to led segments
		//send PORTB the digit to display
		PORTB = i;
		PORTA = dec_to_7seg[segment_data[x]];
		x++;
		_delay_ms(10);
	}
*/
}//while
return 0;
}//main
