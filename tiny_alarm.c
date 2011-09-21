/*

    TinyAlarm - ATTiny45V based alarm

    TinyAlarm is an very simple interrupt based alarm system.

    When first powered on, the alarm beeps for a duration defined by the
    constant STARTUP_DELAY, indicating that the alarm will soon be armed.
    During this time, it will not react to external stimuli.

    After this initial delay, the system sleeps, waiting for a pin-change
    interrupt (on pin 5 by default). When a change is detected on this pin, the
    alarm enters an initial alert phase during which it beeps to indicate it
    has been activated. An indicator LED is also activated. The length of this
    phase is defined by the constant ALERT_1_DURATION.

    After the second delay, the alarm becomes fully active and the alarm siren
    goes off for a period defined by ALERT_2_DURATION.

    Notes:
     * The duration constants have units of 0.5s, or the watchdog timeout
       period if modified.
     * The count variable is an unsigned integer. This means the maximum
       duration is 255/2 seconds. Count could be made 'long' if necessary.
     * Only one pin-change interrupt is active by default. A second could be
       added by modifying the *_pcie functions and adding an ISR. If you do
       this, remember to update which pull-up resistors you activate.
     * Finally, I'm not sure I've implemented the sleep functionality correctly!

*/

#define F_CPU 1000000UL  /* 1 MHz Internal Oscillator */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <avr/sleep.h>

#define STARTUP_DELAY 30
#define ALERT_1_DURATION 30
#define ALERT_2_DURATION 30

#define ALARM_PIN PB4
#define LED_PIN PB3

enum states {
    STARTUP,
    ARMED,
    ALERT_1,
    ALERT_2,
    ALERT_3,
};

volatile enum states state = STARTUP;
volatile unsigned int count = 0;

/*  Enable hardware interrupt. */
void enable_pcie()
{
    GIMSK |= 1<<PCIE;
    PCMSK |= 1<<PCINT0;
}

/*  Disable the external interrupt. */
void disable_pcie()
{
    GIMSK &= ~(1<<PCIE);
    PCMSK &= ~(1<<PCINT0);
}

/* Enable WDT interrupt */
void enable_wdie()
{
    WDTCR |= _BV(WDIE);
}

/* Disable WDT interrupt */
void disable_wdie()
{
    WDTCR &= ~_BV(WDIE);
}

void toggle_alarm()
{
    PORTB ^= _BV(ALARM_PIN);
}

void alarm_off()
{
    PORTB &= ~_BV(ALARM_PIN);
}

void alarm_on()
{
    PORTB |= _BV(ALARM_PIN);
}

void led_on()
{
    PORTB |= _BV(LED_PIN);
}

/* This function does all the work. It is only called by the WDT interrupt.
 * When the chip is woken from sleep by a pin change, it enabled the WDT
 * interrupt, rather than call this directly.
 */
void act()
{
    /* Many things here are duration based, so ensure the number of interrupts is tracked.*/
    count++;

    /*  Startup state. Beep for a while. */
    switch(state) {
        case  STARTUP:
            /*  When we get here, transition to the ARMED state and disable the clock. */
            if (count == STARTUP_DELAY) {

                /*  Disable WDT interrupt */
                disable_wdie();

                /* Enable pin change interrupt - alarm switch, etc. */
                enable_pcie();

                /* Make sure the alarm doesn't stay on */
                alarm_off();

                /*  Reset count for the next state which needs it. */
                count = 0;
                state = ARMED;
            }
            /*  Otherwise we're still in STARTUP, so toggle the alarm. */
            else {
                toggle_alarm();
            }
            break;
        case ARMED:
            /* Nothing to do here. The interrupt updates the state to ALERT_1 */
            break;
        case ALERT_1:
            /*  Enable the indicator LED - stays on forever now :) */
            led_on();

            /*  Beep for a while. Then Transition to the next state */
            /*  Leave the clock running. */
            if (count == ALERT_1_DURATION) {
                state = ALERT_2;
                /*  Enable the alarm: */
                alarm_on();

                /*  Reset count for the next state which needs it. */
                count = 0;
            }
            /*  Otherwise we're still in ALERT_1, so blink. */
            else {
                toggle_alarm();
            }
            break;
        case ALERT_2:
                /*  The LED was switched on at the end of the last state. Leave it for now. */
                if (count == ALERT_2_DURATION) {
                    /*  Now disable the clock, and transition to the final state */
                    disable_wdie();

                    state = ALERT_3;
                    alarm_off();

                    /*  Reset count for the next state which needs it. */
                    count = 0;
                }
                break;
        case ALERT_3:
                /*  Nothing specific happens here. The LED should be on. */
                break;
    }
}

/*  WDT Timeout Interrupt */
ISR(WDT_vect) {
    act();
}

/*  External interrupt 0 */
ISR(PCINT0_vect) {
    /* Disable the interrupt. We can only be triggered once. */
    disable_pcie();

    /* Mark the fact that the alarm has been toggled */
    state = ALERT_1;

    /*  re-enable the clock */
    enable_wdie();
}

/* Initialise WDT and interrupts */
void init (){
    /* ALARM_PIN is digital output */
    DDRB |= _BV (ALARM_PIN);
    /* PB3 is digital output */
    DDRB |= _BV (LED_PIN);

    /* Enable interal pull-up resitors for unused pins (6 and 7) */
    PORTB |= _BV(PB1);
    PORTB |= _BV(PB2);

    /* Set up the watchdog timer - ~0.5s timeout */
    WDTCR = (1<<WDP2) | (1<<WDP0) | (1<<WDIE);

    /* Enable watchdog interrupt */
    enable_wdie();

    /* Use the lowest power sleep mode */
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

/* Start and go to sleep */
int main (void)
{
    cli();
    init();
    sei();

    while (1) {
        sleep_mode();
    }
}
