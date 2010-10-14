// lab1_code.c
// R. Traylor
// 7.21.08

//This program increments a binary display of the number of button pushes on switch 
//S0 on the mega128 board.

#define F_CPU 16000000UL     //16Mhz clock
#include <avr/io.h>
#include <util/delay.h>

//*******************************************************************************
//                            debounce_switch                                  
// Adapted from Ganssel's "Guide to Debouncing"            
// Checks the state of pushbutton S0 It shifts in ones till the button is pushed. 
// Function returns a 1 only once per debounced button push so a debounce and toggle 
// function can be implemented at the same time.  Expects active low pushbutton on 
// Port D bit zero.  Debounce time is determined by external loop delay times 12. 
//*******************************************************************************
int8_t debounce_switch() {
  static uint16_t state = 0; //holds present state
  state = (state << 1) | (! bit_is_clear(PIND, 0)) | 0xE000;
  if (state == 0xF000) return 1;
  return 0;
}

//*******************************************************************************
// Check switch S0.  When found low for 12 passes of "debounc_switch(), increment
// PORTB.  This will make an incrementing count on the port B LEDS. 
//*******************************************************************************
int main()
{
DDRB = 0xFF;  //set port B to all outputs
while(1){     //do forever
	if(debounce_switch()) {
		//Resets PortB if we get to 99
		if(PORTB==0x99){
			PORTB=0;
		}
		else {
			if((PORTB&0x0F)==9){
				//stop incrementing the 4LSBs if we reach 9 and clear them
				//add 2^4 to portb to increment the 10s place
				PORTB=PORTB+0x10;
				PORTB=PORTB&0xF0;
			}
			else
				PORTB++;
		}
	}  //if switch true for 12 passes, increment port B
	_delay_ms(2);                    //keep in loop to debounce 24ms
} //while
} //main
