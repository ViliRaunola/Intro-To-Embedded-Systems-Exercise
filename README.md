# Intro-To-Embedded-Systems-Exercise

## USART

Usart can be used for both in Mega and Uno for sending data to computer through USB. On the computer can be read by using Putty, for example. Added for debugging purposes so we can check the values etc.

Can be used like:
```
printf("Hello World\n\r");
```


## Buzzer (PWM) 
The tone can be set using where the first argument is pre-scaler and the second is the desired tone.
```
OCR1A = topCalculation(1, 700);
```
To start the buzzer in the while loop use:
```
TCCR1B |= (1 << CS10);
```
To turn off the buzzer:
```
TCCR1B &= ~(1 << CS10);
```
