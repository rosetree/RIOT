#ifndef WINCH
#define WINCH

#include "servo.h"

typedef struct {
	servo_t *servo; //Servo-Element, das gesteuert werden soll
	int d; //Durchmesser der Spindel in mm
	int U; //Umfang der Spindel in mm
	gpio_t reed; //Reed-Switch (aktuell ohne Funktion)
} winch_t;

winch_t winch;

void winch_control (winch_t *winch, int l_ges);

#endif
