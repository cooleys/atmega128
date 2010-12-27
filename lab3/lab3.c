// lab3.c
// Sarah Cooley
// ECE473 Lab3 - Fall2010

//	HARDWARE SETUP:
//	PORTA is connected to the segments of the LED display. and to the pushbuttons.
//	PORTA.0 corresponds to segment a, PORTA.1 corresponds to segement b, etc.
//	PORTB bits 4-6 go to a,b,c inputs of the 74HC138.
//	PORTB bit 7 goes to the PWM transistor base.
//  PORTB bit 1-2 control bargraph

#define F_CPU 16000000 // cpu speed in hertz
#define TRUE 1
#define FALSE 0
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>


//holds data to be sent to the segments. logic zero turns segment on
uint8_t segment_data[5]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//decimal to 7-segment LED display encodings, logic "0" turns on segment
uint8_t dec_to_7seg[12] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80,
	0x90, 0xFC, 0xFF};


void tcnt0_init(void)
{
	ASSR   |=  (1<<AS0);    //ext osc TOSC
	TIMSK  |=  (1<<TOIE0);  //enable timer/counter0 overflow interrupt
	TCCR0  |=  (0<<WGM01) | (0<<WGM00) | (0<<COM00) | (0<<COM01) | (0<<CS02) |
				(0<<CS01) | (1<<CS00);
	//normal mode, no prescale

	//timer counter 0 setup, running off i/o clock
	//TCCR0 |= (1<<CS02) | (1<<CS00);  //normal mode, prescale by 128
}

void spi_init(void)
{
	//SPI setup, master mode, clock=clk/2, cycle half phase, low polarity, MSB first
	//interrupts disabled, poll SPIF bit in SPSR to check xmit completion
	DDRB   = DDRB | 0x07;           //Turn on SS, MOSI, SCLK
	SPCR  |= (1<<SPE) | (1<<MSTR);  //set up SPI mode
	SPSR  |= (1<<SPI2X);            // double speed operation
}//spi_init

//*****************************************************************************
//                           timer/counter0 ISR
//When the TCNT0 overflow interrupt occurs, the count_7ms variable is
//incremented.  Every 7680 interrupts the minutes counter is incremented.
//tcnt0 interrupts come at 7.8125ms internals.
// 1/32768         = 30.517578uS
//(1/32768)*256    = 7.8125ms
//(1/32768)*256*64 = 500mS
//*****************************************************************************

ISR(TIMER0_OVF_vect){
  static uint8_t count_7ms = 0;        //holds 7ms tick count in binary
  static uint8_t display_count = 0x01; //holds count for display

  count_7ms++;                        //increment count every 7.8125 ms
  if ((count_7ms % 64) == 0){         //64 interrupts equals one half second
    SPDR = display_count;             //send to display
    while (bit_is_clear(SPSR,SPIF)){} //wait till data is sent out

	PORTB = 0x60;                     //strobe output data reg in HC595 (falling edge)
    PORTB = 0x00;                     //rising edge

	display_count = (display_count << 1); //shift for next time
  }
  if (display_count == 0x00){display_count=0x01;} //set display back to first positon
}


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
uint8_t mode = 0;

DDRB = 0x77; //set portB as outputs

tcnt0_init();  //initalize counter timer zero
spi_init();    //initalize SPI port
sei();

while(1){}
/*
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
			if(i==1 || i==2){
				count += (1<<i);
				//bound the count to 0 - 1023
				if(count > 1023)
					count=count-1023;
			}
			else if(i==0){
				if((mode & 0x0f) == 0x00)
					mode |= 0x0f;
				else
					mode &= 0xf0;
			}
			else if(i==3){
				if((mode & 0xf0) == 0x00)
					mode |= 0xf0;
				else
					mode &= 0x0f;
			}
		}
	}


	//break up the disp_value to 4, BCD digits in the array: call (segsum)
	segsum(count);

	//make PORTA an output
	DDRA = 0xFF;

	int x=0;
	//bound a counter (0-4) to keep track of digit to display
	for(i=0x00; i<0x50; i += 0x10){
		//send 7 segment code to led segments
		//send PORTB the digit to display
		PORTB = i;
		PORTA = dec_to_7seg[segment_data[x]];
		x++;
		_delay_ms(2);
	}
}//while*/
return 0;
}//main
