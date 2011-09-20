/*
    File:       main.c
    Version:    1.0.0
    Date:       May. 12, 2006
    
    TinyAlarm - ATTiny45V based alarm
    
*/
 
#define F_CPU 1000000UL  /* 1 MHz Internal Oscillator */
 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <avr/sleep.h>

enum states {
    STARTUP,
    ARMED,
    ALERT_1,
    ALERT_2,
    ALERT_3,
};

volatile enum states state = STARTUP;
volatile int count = 0;
volatile int toggled = 0;


#define STARTUP_DELAY 30
#define ALERT_1_DURATION 30
#define ALERT_2_DURATION 30

// Timer 0 overflow. Called (1e6/256)/1024 times per second (~3.8)
ISR(TIM0_OVF_vect) {    
    count++;
   
    // Startup state. Beep for a while.
    switch(state) {
        case  STARTUP: 
            // When we get here, transition to the ARMED state and disable the clock.
            // Should also sleep_mode
            if (count == STARTUP_DELAY) {

                TIMSK = 0;

                // Enable hardware interrupt.
                GIMSK = 1<<INT0;
               
                // Alarm off.
                PORTB &= ~_BV(PB4);                

                // Reset count for the next state which needs it.
                count = 0;
                state = ARMED;
            }
            // Otherwise we're still in STARTUP, so toggle the alarm.
            else {
                PORTB ^= _BV(PB4);
            }
            break;
        case ARMED:
            if (toggled == 1) {
                state = ALERT_1;
            }
            break;
        case ALERT_1:
                // Enable the indicator LED - stays on forever now :)
                PORTB |= _BV(PB3);
                // Disable the hardware interrupt
                GIMSK = 0;

                // Beep for a while. Then Transition to the next state
                // Leave the clock running.
                if (count == ALERT_1_DURATION) {
                    state = ALERT_2;
                    // Enable the alarm:
                    PORTB |= _BV(PB4);

                    // Reset count for the next state which needs it.
                    count = 0;
                }
                // Otherwise we're still in ALERT_1, so blink.
                else {
                    PORTB ^= _BV(PB4);
                }
                break;
                // In ALERT_2 we just make a noise for a while
        case ALERT_2:
                // The LED was swtiched on at the end of the last state. Leave it for now.
                if (count == ALERT_2_DURATION) {
                    // Now disable the clock, and transition to the final state
                    TIMSK = 0; 
                    PORTB &= ~_BV(PB4);                
                    state = ALERT_3;

                    // Reset count for the next state which needs it.
                    count = 0;
                }
                break;
        case ALERT_3:
                // Nothing specific happens here. The LED should be on.
                //sleep_mode();
                break;
    }
}

// External interrupt 0
ISR(INT0_vect) {
    toggled = 1;
    // re-enable the clock
    TIMSK  = 1<<TOIE0; 
}


void init (){
    /* PB4 is digital output */
    DDRB = _BV (PB4);   
    /* PB3 is digital output */
    DDRB |= _BV (PB3);   
    
    /*
     * TIMER SETUP
     */
    

    // Timer clock = system clock / 1024
    TCCR0B = (1<<CS02) | (1<<CS00);
   
    // Clear pending interrupts
    TIFR = 1<<TOV0; 

    // Enable Timer Overflow interrupt
    TIMSK  = 1<<TOIE0; 

    // Bits 0 and 1 of the MCUCR control the external interrupt. 
    // 00 -> trigger on low
    // 01 -> trigger on any
    // 10 -> trigger on fall
    // 11 -> triggers on rising 
    
    // Trigger on falling
   MCUCR = (1<<ISC01) | (0<<ISC00);

   set_sleep_mode(SLEEP_MODE_IDLE);


}

int main (void)
{
    init();
    sei(); 
    
    while(1) {}
}
