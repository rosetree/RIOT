#ifndef WINCH
#define WINCH

#include "servo.h"

typedef struct {
	servo_t *servo;
	int d; //Durchmesser der Spindel in mm
	int U;
	gpio_t reed;
} winch_t;

winch_t winch;

void winch_control (winch_t *winch, int l_ges);

#endif