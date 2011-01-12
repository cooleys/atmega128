// lab4.c
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

volatile uint8_t flag = 0;
volatile uint8_t mode = 0;

volatile uint8_t e1 = 0;
volatile uint8_t e2 = 0;

void tcnt0_init(void)
{
	ASSR   |=  (1<<AS0);    //ext osc TOSC
	TIMSK  |=  (1<<TOIE0);  //enable timer/counter0 overflow interrupt
	TCCR0  |=  (0<<WGM01) | (0<<WGM00) | (0<<COM00) | (0<<COM01) | (0<<CS02) |
				(0<<CS01) | (1<<CS00);  //normal mode, no prescale
}

void spi_init(void)
{
	//SPI setup, master mode, clock=clk/2, cycle half phase, low polarity, MSB first
	//interrupts disabled, poll SPIF bit in SPSR to check xmit completion
	DDRB   = DDRB | 0x07;           //Turn on SS, MOSI, SCLK
	SPCR  |= (1<<SPE) | (1<<MSTR);  //set up SPI mode
	SPSR  |= (1<<SPI2X);            // double speed operation
}//spi_init

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

//*****************************************************************************
//                           timer/counter0 ISR
//When the TCNT0 overflow interrupt occurs, the count_7ms variable is
//incremented.  Every 7680 interrupts the minutes counter is incremented.
//tcnt0 interrupts come at 7.8125ms internals.
// 1/32768         = 30.517578uS
//(1/32768)*256    = 7.8125ms
//*****************************************************************************

ISR(TIMER0_OVF_vect){
	static uint8_t i=0;
	static uint8_t data=0;
	static uint8_t state=0;

	//Wiggle P0	PORTB |= 0x01;
	PORTB &= 0xFE;

	//make PORTA an input port with pullups
	DDRA = 0x00;
	PORTA = 0xFF;

	PORTB = 0x7F; //enable tristate buffer
	state=chk_buttons(state);

	data = SPDR;
	e1 = data & 0x03;
	e2 = (data & 0x0C) >> 2;
	mode ^= state & 0x03;

	SPDR = mode | (e1 << 3) | (e2 << 6);
	while(bit_is_clear(SPSR, SPIF)){}
	PORTB = 0x60;
	PORTB = 0x00;

	PORTB = 0x0F; //disable tristate buffer
	DDRA = 0xFF;
	flag = 1;
}


//******************************************************************************
uint8_t main()
{
uint8_t i=0;
uint16_t count=0;
uint8_t state = 0;
uint8_t turned = 0;

uint8_t ee1 = 0;
uint8_t ee2 = 0;

uint8_t left_turn[4] = {3, 1, 0, 2};
uint8_t right_turn[4] = {3, 2, 0, 1};

DDRB = 0x77; //set portB as outputs

tcnt0_init();  //initalize counter timer zero
spi_init();    //initalize SPI port
sei();

while(1){
	if(flag && (ee1 != e1 || ee2 != e2) ){
		if((ee1 == 3 && e1 == 2) || (ee2 == 3 && e2 == 2)){
			switch(mode){
				case 0: //encoders increment by 1
					count += 1;
					break;
				case 1: //encoders increment by 2
					count += 2;
					break;

				case 2://encoders increment by 4
					count += 4;
					break;
			}
			if(count > 1023)
				count = 0;
		}

		if((ee1 == 3 && e1 == 1) || (ee2 == 3 && e2 == 1)){
			switch(mode){
				case 0: //encoders increment by 1
					count -= 1;
					break;
				case 1: //encoders increment by 2
					count -= 2;
					break;

				case 2://encoders increment by 4
					count -= 4;
					break;
			}

			if(count > 1024)
				count = 1023;
		}

		ee1 = e1;
		ee2 = e2;
		flag =0;
	}

	//make PORTA an output
	DDRA = 0xFF;

	segsum(count);
	int x=0;
	//bound a counter (0-4) to keep track of digit to display
	for(i=0x00; i<0x50; i += 0x10){
		//send 7 segment code to led segments
		//send PORTB the digit to display
		PORTB = i;
		PORTA = dec_to_7seg[segment_data[x]];
		x++;
		_delay_ms(1);
	}
}//while
return 0;
}//main
