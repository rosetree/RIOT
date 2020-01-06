#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "board.h"
#include "xtimer.h"
#include "periph/pwm.h"
#include "servo.h"
#include "math.h"
#include "winch.h"
#include "periph/gpio.h"

#include "shell.h"


#define PI 3.14159265358979323846


#define STILLSTAND	(1500U) //Bei dieser Zeit steht der Motor still. Sollte dies nicht der Fall sein muss er kalibriert werden.
#define MAXGESCHW	(1300U) //Bei dieser Zeit fährt der Motor auf voller Geschwindigkeit vorwärts
#define MINGESCHW	(1700U) //Bei dieser Zeit fährt der Motor auf voller Geschwindigkeit rückwärts

//#define MINGESCHW	(2680U)

void winch_init (winch_t *winch, servo_t *servo, int d, gpio_t reed)
{
    gpio_init(reed, GPIO_IN_PD);

    winch->servo = servo;
    winch->d = d;
    winch->U = d * PI;
    winch->reed = reed;
}


void winch_control (winch_t *winch, int l_ges)
{


	int t_anfahrt = 2000; //Zeit zum Anfahren in ms
	int t_mitte = -1; // Zeit, die während der konstanten Periode vergeht in ms
	int t_step; //Zeit die für jeden Anfahr-Schritt benötigt wird in ms
	int l_anfahrt; //Länge, die bei der Anfahrt zurückgelegt wird in mm
	int l_mitte; //Länge, die während der konstanten Periode zurückgelegt wird in mm
	while (t_mitte<0)
	{
		t_step = (int)(t_anfahrt/7); 
		//l_anfahrt = SUMME(f_step*t_step*U) = SUMME(f_step)*t_step*U
		//SUMME(f_step)= 10/50200ms + 10/28600ms + 10/19600ms + 10/14900ms + 10/14100ms + 10/13300ms + 10/13100ms + 10/13000ms = 0.004723888
		l_anfahrt = (int)(0.004723888*t_step* winch->U); 
		l_mitte = (int)(l_ges - l_anfahrt*2); 
		t_mitte = l_mitte / (0.000769231* winch->U); 
		if (t_mitte < 0) t_anfahrt -= 100;
	}

	int dc=1500;
	unsigned long int zeit;


	if (l_ges>0) //positive direction
	{
		for (dc = 1500; dc>=1100;dc-=50)
		{
			servo_set(winch->servo, dc);
			zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
			while (xtimer_now_usec() < (zeit+t_step*1000));
		}

		zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
		while (xtimer_now_usec() < (zeit+t_mitte*1000));

		for (dc = 1100; dc<=1500;dc+=50)
		{
			servo_set(winch->servo, dc);
			zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
			while (xtimer_now_usec() < (zeit+t_step*1000));
		}		

	}

	else //negative direction
	{
		for (dc = 1500; dc<=1900;dc+=50)
		{
			servo_set(winch->servo, dc);
			zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
			while (xtimer_now_usec() < (zeit-t_step*1000));
		}

		zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
		while (xtimer_now_usec() < (zeit-t_mitte*1000));

		for (dc = 1900; dc>=1500;dc-=50)
		{
			servo_set(winch->servo, dc);
			zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
			while (xtimer_now_usec() < (zeit-t_step*1000));
		}		
	}


}