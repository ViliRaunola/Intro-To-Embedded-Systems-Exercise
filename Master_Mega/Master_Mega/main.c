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
send_command_to_slave(char *spi_data_to_send)
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
comparePassword(char *user_input, int *state)
{
	int compare_result;
	
	compare_result = strcmp(PASSWORD, user_input);
	
	if(compare_result)
	{
		printf("Wrong password");
		// Clearing the screen
		send_command_to_slave("4");
		
		send_command_to_slave("3>Try again:");
		// Clearing the user input
		user_input[0] = '\0';
	}else
	{
		printf("Passwords match!\n\r");
		// Clearing the user input
		user_input[0] = '\0';
		*state = STOP_TIMER;
	}
}

// Appends a character to a character array
void
appendCharToCharArray(char* array, char c)
{
	int len = strlen(array);
	array[len] = c;
	array[len+1] = '\0';
}


// Creating the payload to print given user input to LCD
void
createUserInputString(char *stars_to_print_command, int *user_input_len)
{
	int i = 0;
	for (i = 2; i < *user_input_len+2; i++)
	{
		stars_to_print_command[i] = '*';
	}
}

// Reads the pressed key and appends it to the user input
void
getPassword(char *user_input, int *state){
	
	char key_pressed;
	int user_input_len;
	char stars_to_print_command[CHAR_ARRAY_SIZE] = "5>";
	
	printf("Type Something: ");
	KEYPAD_Init();
	key_pressed = KEYPAD_GetKey();
	printf("%c\n\r", key_pressed);
	
	appendCharToCharArray(user_input, key_pressed);
	printf("Current user input: %s\n\r", user_input);
	
	user_input_len = strlen(user_input);
	
	createUserInputString(stars_to_print_command, &user_input_len);
	
	send_command_to_slave(stars_to_print_command);
	
	if(user_input_len == 4)
	{
		comparePassword(user_input, state);
	}
	
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
	
	// The user input from keypad is appended to this char array
	char user_input[CHAR_ARRAY_SIZE] = "\0";
	
	// Clearing the screen
	send_command_to_slave("4");
	
    while (1) 
    {	
		switch(state)
		{
			case WAIT_MOVEMENT:
				// Waiting for movement
				motionSense(MOTION_SENSOR_PIN);
				
				// Movement detected --> sending message to lcd
				send_command_to_slave("3>Give pass code:");
				
				// Switching state to receive the password
				state = KEYPAD_INPUT;
				break;
				
			case START_TIMER:
				break;
				
			case KEYPAD_INPUT:
				getPassword(user_input, &state);
				break;
				
			case STOP_TIMER:
				send_command_to_slave("3>Alarm disarmed");
				state = WAIT_MOVEMENT;
				break;
				
			default:
				//add something here
				break;
		}
    }
}


/*strcpy(g_spi_data_to_send, "1");
		send_command_to_slave(g_spi_data_to_send);
		printf("Buzzer on command sent. Sleeping for 5s\n\r");
		_delay_ms(5000);
		
		strcpy(g_spi_data_to_send, "2");
		send_command_to_slave(g_spi_data_to_send);
		printf("Buzzer off command sent. Sleeping for 5s\n\r");
		_delay_ms(5000);
		
		strcpy(g_spi_data_to_send, "3:Hello!");
		send_command_to_slave(g_spi_data_to_send);
		printf("Print to screen command sent. Sleeping for 5s\n\r");
		_delay_ms(5000);
		
		strcpy(g_spi_data_to_send, "4");
		send_command_to_slave(g_spi_data_to_send);
		printf("Clear screen command sent. Sleeping for 5s\n\r");
		_delay_ms(5000);
*/

/*#########################################################EOF#########################################################*/

