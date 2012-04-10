// suitpajamas.c
// Sarah Cooley
// Last Modified 4/10/2011

/*
* Reads the prox sensor at F0 and displays pretty lights based on that value 
*  
*/

#define F_CPU 16000000UL	   //16Mhz clock
#include <avr/io.h>
#include <util/delay.h>

void adc_init(){
	// Set ADC prescalar to 128 - 125KHz sample rate @ 16MHz
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	// Set ADC reference to AVCC and left adjust for easy reading 
	ADMUX |= (1 << REFS0) | (1<<ADLAR);
	// Set ADC to free running mode and enable
	ADCSRA |= (1 << ADFR) | (1 << ADEN);
}

void ir_init(){
}

int main()
{
DDRB = 0xFF;	//set port B (LED lights) to all outputs
DDRF = 0x00;	//set port F (prox sensor) to inputs
PORTB = 0x00;	//Deavtivate pullup resistor on this input so it'll default low.

adc_init();
ADCSRA |= (1 << ADSC);  // Start A2D Conversions

uint16_t dist = 0; 

while(1){	   //do forever
	dist = ADCH;

	PORTB = (1 << dist / (255/8)) - 1;
	
	_delay_ms(2);

} //while
} //main
