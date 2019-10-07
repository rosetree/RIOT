/*
 * Copyright (C) 2015 Eistec AB
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief       Test for servo driver
 *
 * This test initializes the given servo device and moves it between
 * 1.000 -- 2.000 ms, roughly -/+ 90 degrees from the middle position if the
 * connected servo is a standard RC servo.
 *
 * @author      Joakim Nohlgård <joakim.nohlgard@eistec.se>
 *
 * @}
 */

#include <stdio.h>

#include "cpu.h"
#include "board.h"
#include "xtimer.h"
#include "periph/pwm.h"
#include "servo.h"

#define DEV         PWM_DEV(0)
#define CHANNEL     0
#define SERVO_MIN	(500U)
#define SERVO_MAX 	(5500U)


#define VORWAERTS	(1000U)
#define RUECKWAERTS	(5000U)
#define STILL		(2583U)



/* Step size that we move per WAIT us */
#define STEP             (10U)

/* Sleep time between updates, no need to update the servo position more than
 * once per cycle */
#define WAIT             (1000000U)

static servo_t servo;

int main(void)
{
    int res;
   // unsigned int pos = (STEP_LOWER_BOUND + STEP_UPPER_BOUND)/2;
   // int step = STEP;

    puts("\nRIOT RC servo test");
    puts("Connect an RC servo or scope to PWM_0 channel 0 to see anything");

    res = servo_init(&servo, DEV, CHANNEL, SERVO_MIN, SERVO_MAX);
    if (res < 0) {
        puts("Errors while initializing servo");
        return -1;
    }
    puts("Servo initialized.");


    while (1) {
        servo_set(&servo, VORWAERTS);
        xtimer_usleep(WAIT*2);
	servo_set(&servo, STILL);
	xtimer_usleep(WAIT*2);
        servo_set(&servo, RUECKWAERTS);
        xtimer_usleep(WAIT*2);
	servo_set(&servo, STILL);
	xtimer_usleep(WAIT*2);
    }

    return 0;
}
