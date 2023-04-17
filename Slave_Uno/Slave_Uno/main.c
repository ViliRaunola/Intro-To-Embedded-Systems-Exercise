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
#define WAIT_COMMAND 0
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
int 
topCalculation(int prescale, int frequency)
{
	return (F_CPU / 2 * prescale * frequency);
}

// Waits for the command to be sent from the mega
void
receive_command_from_mega(int *state, char *delimeter, char *payload)
{
	// To this array data is saved from Master in SPI communication
	unsigned char spi_data_to_receive[CHAR_ARRAY_SIZE];
	
	int temp_state;
	for (int8_t i = 0; i < CHAR_ARRAY_SIZE; i++)
	{
		// Checking SPI status register for reception complete
		while(!(SPSR & (1 << SPIF))) {;}
		// Getting the data from the register (Data from Mega)
		spi_data_to_receive[i] = SPDR;
	}
	
	printf("Data: %s\n\r", spi_data_to_receive);
	// Splitting the string using : so the command and payload can be separated
	char *ptr_split = strtok(spi_data_to_receive, delimeter);
	
	// Converting command string to integer
	sscanf(ptr_split, "%d", &temp_state);
	
	// Saving the command to state
	*state = temp_state;
	
	// Getting the payload
	ptr_split = strtok(NULL, delimeter);
	
	// Copying the payload if there was any
	if(ptr_split != NULL) {
		strcpy(payload, ptr_split);
	}
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
	
	// Storing the possible payload that comes with the command from Mega
	unsigned char payload[CHAR_ARRAY_SIZE];
	
	/* 
	The state of the Uno, used in the switch case structure. 
	Is initialized as waiting for command since Uno just listens to Mega's commands.
	*/
	int state = WAIT_COMMAND; 
	
	// Delimeter for splitting the command and payload
	char delimeter[2] = ">";
	
	
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
		/* 
		The command to run is received from the mega.
		The correct action is decided in the switch case 
		*/
		switch(state)
		{
			case WAIT_COMMAND:
				receive_command_from_mega(&state, delimeter, payload);
				break;
			
			case BUZZER_ON:
				// Turns the buzzer on
				TCCR1B |= (1 << CS10);
				state = WAIT_COMMAND;
				break;
			
			case BUZZER_OFF:
				// Turns the buzzer off
				TCCR1B &= ~(1 << CS10);
				state = WAIT_COMMAND;
				break;
			
			case DISPLAY:
				// Getting the next part aka the payload of command
				lcd_clrscr();
				lcd_puts(payload);
				state = WAIT_COMMAND;
				break;
				
			case DISPLAY_CLEAR:
				lcd_clrscr();
				state = WAIT_COMMAND;
				break;
			
			default:
				// Add something
				break;
		}
		
    }
}

/*#########################################################EOF#########################################################*/


