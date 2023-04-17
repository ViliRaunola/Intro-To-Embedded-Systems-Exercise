/*
 * Master_Mega.c
 *
 * Created: 23/03/2023 13:56:38
 * Author : Group 07
 */ 

#define F_CPU 16000000UL
#define FOSC 16000000UL
#define BAUD 9600
#define MYUBRR (FOSC/16/BAUD-1)
#define CHAR_ARRAY_SIZE 40
#define PASSWORD "1234"
#define MOTION_SENSOR_PIN PB4 //pin D10 (PB4) from Arduino Mega for sensor
#define OK_BUTTON_CHAR 'A'

/*Definitions to switch cases*/
#define WAIT_MOVEMENT 0
#define START_TIMER 1
#define KEYPAD_INPUT 2
#define STOP_TIMER 3

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "keypad.h"








/* USART_... Functions are for 
communicating between the Arduino and the computer through the USB.
Can be used by printf();. */

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

/*
This function sends the command in the char array to the slave Uno to be executed using SPI.
*/
void
send_command_to_slave(unsigned char *spi_data_to_send)
{
	PORTB &= ~(1 << PB0); // SS low --> enables slave device
	
	//Sending the data to the slave
	for (int8_t i = 0; i < CHAR_ARRAY_SIZE; i++)
	{
		// Sending one byte at a time
		SPDR = spi_data_to_send[i];
		// Delays are added to prevent things happening too fast
		_delay_us(10);
		// Checking SPI status register if the transmit is complete
		while (!(SPSR & (1 << SPIF))) {;}
		_delay_us(10);
	}
	PORTB |= (1 << PB0); // SS high --> disable slave device
}

FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);



//Function for motion detection
static void
motionSense(int sensePin){
	
	int sensorState = 0; // current state of pin
	
	while(1){
		
		sensorState = (PINB & (1 << sensePin));
		
		if(sensorState != 0){
			
			printf("Motion Detected\n\r");
			break;
		}
		sensorState = 0;
	}
}

/*
Compares the user input after OK is pressed to the stored password.
If the passwords match the state is switched.
*/
void
comparePassword(unsigned char *user_input, int *state, unsigned char *spi_data_to_send)
{
	int compare_result;
	
	compare_result = strcmp(PASSWORD, user_input);
	
	if(compare_result)
	{
		strcpy(spi_data_to_send, "3|Give pass code:");
		send_command_to_slave(spi_data_to_send);
	}else
	{
		*state = STOP_TIMER;
	}
}

// Appends a character to a character array
void
appendCharToCharArray(unsigned char* array, char c)
{
	int len = strlen(array);
	array[len] = c;
	array[len+1] = '\0';
}

// Reads the pressed key and appends it to the user input
static void
getPassword(unsigned char *user_input, int *state, unsigned char *spi_data_to_send){
	
	printf("Type Something: ");
	char keyPressed;
	KEYPAD_Init();
	keyPressed = KEYPAD_GetKey();
	printf("%c\n\r", keyPressed);
	
	
	if(keyPressed == OK_BUTTON_CHAR)
	{
		comparePassword(user_input, state, spi_data_to_send);
	}
	
	appendCharToCharArray(user_input, keyPressed);
	printf("Current user input: %s\n\r", user_input);
}





int main(void)
{
    // Initializing the USART
	USART_Init(MYUBRR);
	stdout = &uart_output;
	stdin = &uart_input;
	
	// Setting SS, MOSI and SCL as outputs
	DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2);
	
	// Set the SPI on and make the mega master
	SPCR |= (1 << SPE) | (1 << MSTR);
	
	// Set SPI clock to 1 MHz
	SPCR |= (1 << SPR0);
	
	// Setting input from motion sensor
	DDRB &= ~(1 << MOTION_SENSOR_PIN);
	
	/* 
	The state of Mega, used in the switch case structure. 
	Is initialized as waiting for movement.
	*/
	int state = WAIT_MOVEMENT; 
	
	// Stores the data that is sent from Mega to  Uno
	unsigned char spi_data_to_send[CHAR_ARRAY_SIZE] = "1";
	
	// The user input from keypad is appended to this char array
	unsigned char user_input[CHAR_ARRAY_SIZE] = "\0";
	
    while (1) 
    {	
		switch(state)
		{
			case WAIT_MOVEMENT:
				// Waiting for movement
				motionSense(MOTION_SENSOR_PIN);
				
				// Movement detected --> sending message to lcd
				strcpy(spi_data_to_send, "3|Give pass code:");
				send_command_to_slave(spi_data_to_send);
				
				// Switching state to receive the password
				state = KEYPAD_INPUT;
				break;
				
			case START_TIMER:
				break;
				
			case KEYPAD_INPUT:
				getPassword(user_input, &state, spi_data_to_send);
				break;
				
			case STOP_TIMER:
				break;
				
			default:
				//add something here
				break;
		}
    }
}


/*strcpy(spi_data_to_send, "1");
		send_command_to_slave(spi_data_to_send);
		printf("Buzzer on command sent. Sleeping for 5s\n\r");
		_delay_ms(5000);
		
		strcpy(spi_data_to_send, "2");
		send_command_to_slave(spi_data_to_send);
		printf("Buzzer off command sent. Sleeping for 5s\n\r");
		_delay_ms(5000);
		
		strcpy(spi_data_to_send, "3:Hello!");
		send_command_to_slave(spi_data_to_send);
		printf("Print to screen command sent. Sleeping for 5s\n\r");
		_delay_ms(5000);
		
		strcpy(spi_data_to_send, "4");
		send_command_to_slave(spi_data_to_send);
		printf("Clear screen command sent. Sleeping for 5s\n\r");
		_delay_ms(5000);
*/

/*#########################################################EOF#########################################################*/

