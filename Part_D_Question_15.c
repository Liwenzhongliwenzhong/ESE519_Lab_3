/*
 * Partd.c
 *
 * Created: 2020/10/18 20:36:29
 * Author : liwen
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#define BAUD_RATE 9600
#define BAUD_PRESCALER (((F_CPU / (BAUD_RATE * 16UL))) - 1)

//Set pulse width of the trigger signal of the ultrasonic sensor
int high_time = 25;
//Keep the overflow times
volatile long long ovf = 0;
volatile int ovf_1 = 0;
volatile int ovf_2 = 0;
//High = 1, low = 0
volatile int high_or_low = 0;
//Calculate the pulse width of the echo signal of the ultrasonic sensor
volatile int width = 0;
volatile int edge = 0;
//Calculate the actual distance
volatile int distance = 0;

void Initialize()
{
	cli();
	//Set PD6, PD5 as output pin and PB0 as input pin
	DDRD |= (1<<DDD6);
	DDRB &= ~(1<<DDB0);
	DDRD |= (1<<DDD5);
	//Prescale Timer0 with by 256
	TCCR0B |= (1<<CS00);
	TCCR0B |= (1<<CS01);
	TCCR0B &= ~(1<<CS02);
	//Set Timer0 as phase correct PWM, TOP = OCRA
	TCCR0A |= (1<<WGM00);
	TCCR0A &= ~(1<<WGM01);
	TCCR0B |= (1<<WGM02);
	//Set OCR1A and OCR1B value
	OCR1A = 10;
	OCR1B = 5;
	//Non-inverting, clear on compare match
	TCCR0A |= (1<<COM0B1);
	
	//Prescale Timer1 by 8
	TCCR1B |= (1<<CS11);
	//Set Timer0 as normal
	TCCR1A &= ~(1<<WGM10);
	TCCR1A &= ~(1<<WGM11);
	TCCR1B &= ~(1<<WGM12);
	//Enable output compare interrupt
	TIMSK1 |= (1<<OCIE1A);
	//Enable input capture interrupt
	TIMSK1 |= (1<<ICIE1);
	//Initially looking for falling edge
	TCCR1B |= (1<<ICES1);
	//Enable Timer1 overflow interrupt
	TIMSK1 |= (1<<TOIE1);
	//Initialize OCR1A value
	OCR1A = 10;
	//Clear interrupt flag
	TIFR1 |= (1<<OCF1A);
	sei();
}

ISR(TIMER1_COMPA_vect)
{
	//Generate the trigger signal of the ultrasonic sensor
	if (high_or_low)
	{
		//Toggle PD6, PD6 set to low
		PORTD &= ~(1<<PORTD6);
		high_or_low = 0;
		OCR1A = 0;
	}
	else
	{
		//Make sure the time interval between two pulses is long enough
		if (ovf - ovf_1 > 4)
		{
			//Toggle PD6, PD6 set to high
			PORTD |= (1<<PORTD6);
			high_or_low = 1;
			OCR1A = high_time;
			ovf_1 = ovf;
		}
	}
}

ISR(TIMER1_OVF_vect)
{
	ovf++;
}

ISR(TIMER1_CAPT_vect)
{
	//Calculate pulse width of echo signal
	width = 65536*(ovf - ovf_2) + ICR1 - edge;
	ovf_2 = ovf;
	edge = ICR1;
	//The pulse ends at falling edge
	if (!(TCCR1B & (1<<ICES1)))
	{
		//Calculate distance
		distance = width/118;
		//To ensure correct measurement, we only take distance between 0 and 100cm
		if ((distance > 0)&&(distance < 100))
		{
			//Update PWM frequency
			OCR0A = distance + 58;
			OCR0B = OCR0A/20;
		}
	}
	//Toggle edge trigger type
	TCCR1B ^= (1<<ICES1);
}

int main(void)
{
	Initialize();
	UART_initialize(BAUD_PRESCALER);
	while (1);
}










