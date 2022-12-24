#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// AT tiny 85 fuses -U lfuse:w:0x62:m -U hfuse:w:0xdc:m -U efuse:w:0xff:m
// AT tiny 13 fuses -U lfuse:w:0x7A:m -U hfuse:w:0xFB:m

#if F_CPU == 8000000
#define PWM_TOP 160 // 8mhz clock
#elif F_CPU == 9600000
#define PWM_TOP 188 // 9.6 mhz clock
#else
#error define PWM_TOP
#endif

/*
    8mhz internal oscillator, no 8x clock divider.

    Pin 4 - PB4 - Trimpot input for fan speed control
    Pin 6 - PB1 - PWM output at 25khz.
*/

static void
init_pwm(void)
{
    DDRB = (1<<PB1);

    /* Mode 5 PWM.  TOP is OCR0A */

    TCCR0A = (1<<WGM00)|(1<<COM0B1)|(1<<COM0B0);

    TCCR0B = (1<<CS00)|(1<<WGM02);   //prescaling with 1

    OCR0A = PWM_TOP;    /* Gives us a 25 khz pwm signal */
    OCR0B = PWM_TOP/2;  /* Default to 50% duty cycle */
}


/* ADC running in free mode.  Interrupt on conversion complete. */
static void
init_adc(void)
{
    ADMUX = (1 << MUX1); // ADC 0, PB2. VCC as voltage reference.

    ADCSRB = 0; // Free running mode.
    ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 << ADEN) | (1 << ADIE) | (1 << ADIF) | (1 << ADSC) | (1 << ADATE); // 64 prescaler, ADC Enabled, interrupt.
}

ISR(ADC_vect)
{
    uint8_t low = ADCL;
    uint8_t high = ADCH;
    uint8_t dc;
    uint16_t val = (high << 8) | low;

    float pc = val / 1024.0f;

    dc = PWM_TOP * pc;

    /* Perform some clamping */
    if (dc < (PWM_TOP * 0.05f)) {
        /*If the ADC value is in the 0% to 5% range, set it to zero */
        dc = 0;
    }
    /*
    else if (dc < (PWM_TOP * 0.2f)) {
        dc = PWM_TOP * 0.2f;
    }
    */
    else if (dc > (PWM_TOP * 0.95f) ) {
        /*If in the 95% to 100% range, set it to 100% */
        dc = PWM_TOP;
    }

    /* We're using the inverting output, so we have to invert our result */
    OCR0B = PWM_TOP - dc;
}

int main(void)
{
    init_pwm();
    init_adc();

    sei();
    set_sleep_mode(SLEEP_MODE_ADC);

    while(1) {
        sleep_enable();
    }
}

