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

// Enable hardware interrupt.
void enable_pcie() 
{
    GIMSK |= 1<<PCIE;
    PCMSK |= 1<<PCINT0;
}

// Disable the external interrupt.
void disable_pcie()
{
    GIMSK &= ~(1<<PCIE);
    PCMSK &= ~(1<<PCINT0);
}

void act() 
{
    count++;
   
    // Startup state. Beep for a while.
    switch(state) {
        case  STARTUP: 
            // When we get here, transition to the ARMED state and disable the clock.
            if (count == STARTUP_DELAY) {

                // Disable WDT interrupt
                WDTCR &= ~_BV(WDIE);

                enable_pcie();
               
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
                    WDTCR &= ~_BV(WDIE);
                    PORTB &= ~_BV(PB4);                
                    state = ALERT_3;

                    // Reset count for the next state which needs it.
                    count = 0;
                }
                break;
        case ALERT_3:
                // Nothing specific happens here. The LED should be on.
                break;
    }
}

// WDT Timeout Interrupt
ISR(WDT_vect) {    
    act();
}

// External interrupt 0
ISR(PCINT0_vect) {
    disable_pcie();

    toggled = 1;
    // re-enable the clock
    WDTCR |= _BV(WDIE);
    act();
}

void init (){
    /* PB4 is digital output */
    DDRB = _BV (PB4);   
    /* PB3 is digital output */
    DDRB |= _BV (PB3);   

    // Set up the watchdog timer - ~0.5s timeout 
    WDTCR = (1<<WDP2) | (1<<WDP0) | (1<<WDIE);

    // Enable pin change inerrupts
    enable_pcie();

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

int main (void)
{
    cli();
    init();
    sei(); 

    while (1) {
        sleep_mode();
    }
}
