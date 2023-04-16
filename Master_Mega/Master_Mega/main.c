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
		
		//printf("Waiting for Motion\n");
		
		sensorState = (PINB & (1 << sensePin));
		
		if(sensorState != 0){
			
			printf("Motion Detected\n");
			//Do Something;
		}
		sensorState = 0;
		_delay_ms(1000);
		
	}
	
	
}


static void
getPassword(){
	
	printf("Type Something: ");
	char keyPressed;
	KEYPAD_Init();
	keyPressed = KEYPAD_GetKey();
	printf("%c\n ", keyPressed);
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
	
	unsigned char spi_data_to_send[CHAR_ARRAY_SIZE] = "1";
	
    while (1) 
    {	
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
		getPassword();
    }
}

