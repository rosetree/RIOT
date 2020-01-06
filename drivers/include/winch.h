#ifndef WINCH
#define WINCH


#ifdef __cplusplus
extern "C" {
#endif


#include "servo.h"

typedef struct {
	servo_t *servo;
	int d; //Durchmesser der Spindel in mm
	int U;
	gpio_t reed;
} winch_t;

void winch_init(winch_t *winch, servo_t *servo, int d, gpio_t reed);

void winch_control(winch_t *winch, int l_ges);


#ifdef __cplusplus
}
#endif


#endif