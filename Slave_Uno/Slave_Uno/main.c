/*
 * Slave_Uno.c
 *
 * Created: 23/03/2023 13:59:02
 * Author : Group 07
 */ 

#define F_CPU 16000000UL
#define FOSC 16000000UL
#define BAUD 9600
#define MYUBRR (FOSC/16/BAUD-1)
#define CHAR_ARRAY_SIZE 40

/*Definitions to switch cases*/
#define BUZZER_ON 1
#define BUZZER_OFF 2
#define DISPLAY 3
#define DISPLAY_CLEAR 4


#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <string.h>
#include "lcd.h" // Source: From the provided course material

/*USART*/
/*Source from course material*/

/* 
These functions are for 
communicating between the Arduino and the computer through the USB.
Can be used by printf();. 
*/

static void
USART_Init( uint16_t ubrr)
{
	/* Set baud rate */
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	/* Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Set frame format: 8data, 2stop bit */
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

static void
USART_Transmit( unsigned char data, FILE *stream )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) )
	{
		;// Doing nothing at all
	}
	/* Put data into buffer, sends the data */
	UDR0 = data;
}

static char
USART_Receive( FILE *stream)
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) )
	{
		;// Doing nothing at all
	}
	/* Get and return received data from buffer */
	return UDR0;
}

FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);

/*BUZZER*/
/*Source from course material*/

// Runs when compare matches. Resets the timer back to 0.
ISR(TIMER1_COMPA_vect){
	TCNT1 = 0;
}

// Solves the equation found on Uno manual p.128.
int topCalculation(int prescale, int frequency)
{
	return (F_CPU / 2 * prescale * frequency);
}



int main(void)
{
	// Pin 9 for buzzer
	DDRB |= (1 << PB1);
	
	// Setting MISO as output
	DDRB |= (1 << PB4);
	
	// Set the SPI on
	SPCR |= (1 << SPE);
	
	// Set SPI clock to 1 MHz
	SPCR |= (1 << SPR0);
	
	// To this array data is saved from Master in SPI communication
	unsigned char spi_data_to_receive[CHAR_ARRAY_SIZE];
	int state; 
	
	// Enable interruts, for buzzer.
	sei();
	
	// Initialize counter
	TCCR1A = 0; // Resetting entire register
	TCCR1B = 0; // Resetting entire register
	TCNT1 = 0; // Timer to 0
	TCCR1A |= (1 << COM1A0); // Toggle compare match on
	
	// To mode 9, PWM, Phase and Frequency Correct
	TCCR1B |= (1 << WGM13);
	TCCR1A |= (1 << WGM10);
	
	// Enabling interrupts for timer 1, output compare A
	TIMSK1 |= (1 << OCIE1A); 
	
	/* 
	Setting the right frequency for the buzzer.
	Using 700 MHz, pre-scaler 1.
	*/
	OCR1A = topCalculation(1, 700);
	
    // Initializing the USART
	USART_Init(MYUBRR);
	stdout = &uart_output;
	stdin = &uart_input;
	
	// Initializing the LCD, cursor off, lcd cleared
	lcd_init(LCD_DISP_ON);
	lcd_clrscr();
	
	
	
    while (1) 
    {
		
		for (int8_t i = 0; i < CHAR_ARRAY_SIZE; i++)
		{	
			// Checking SPI status register for reception complete
			while(!(SPSR & (1 << SPIF))) {;}
			// Getting the data from the register (Data from Mega)
			spi_data_to_receive[i] = SPDR;
		}
		printf("Command: %s. From Mega\n\r", spi_data_to_receive);
		
		// Splitting the string using : so the command and payload can be separated
		char *ptr_split = strtok(spi_data_to_receive, ":");
		
		// Converting command string to integer
		sscanf(ptr_split, "%d", &state);
		
		/* 
		The command to run is received from the mega.
		The correct action is decided in the switch case 
		*/
		switch(state)
		{
			case BUZZER_ON:
				TCCR1B |= (1 << CS10);
				break;
			
			case BUZZER_OFF:
				TCCR1B &= ~(1 << CS10);
				break;
			
			case DISPLAY:
				lcd_clrscr();
				lcd_puts("Hello world!");
				break;
			case DISPLAY_CLEAR:
				lcd_clrscr();
				break;
			
			default:
				// Add something
				break;
		}
		
    }
}


