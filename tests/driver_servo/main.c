#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "board.h"
#include "xtimer.h"
#include "periph/pwm.h"
#include "servo.h"
#include "math.h"

#include "shell.h"

#define DEV         PWM_DEV(0)
#define CHANNEL     2
#define SERVO_MIN	(500U)
#define SERVO_MAX 	(5500U)


#define STILLSTAND	(1500U) //Bei dieser Zeit steht der Motor still. Sollte dies nicht der Fall sein muss er kalibriert werden.
#define MAXGESCHW	(1300U) //Bei dieser Zeit fährt der Motor auf voller Geschwindigkeit vorwärts
#define MINGESCHW	(1700U) //Bei dieser Zeit fährt der Motor auf voller Geschwindigkeit rückwärts

//#define MINGESCHW	(2680U)



static servo_t servo;

int d = 50; //Durchmesser der Spindel in mm
int U;



static int _kalibrieren(int argc, char**argv)
{
	if (argc !=1) {
		printf("keine Parameter erforderlich\n");
		printf("%s",argv[0]); //nur um Fehlermeldung zu verhindern
	

	}
	servo_set(&servo, STILLSTAND);
	printf("Stellen Sie mithlfe des Potis den Motor auf Stillstand\n");
	return 0;
}






static int _set(int argc, char**argv)
{
    if (argc != 2) {
        printf("usage: %s <value> \n", argv[0]);
        return 1;
    }

    unsigned dutycycle= atoi(argv[1]);


    servo_set(&servo, dutycycle);
    printf("auf %u gesetzt \n",dutycycle);
    return 0;
}








static int _geschw_neu(int argc, char**argv)
{
	if (argc != 3) 
	{
        	printf("usage: %s <value> <unit> \n", argv[0]);
	        return 1;
    	}

	int l_ges = atoi(argv[1]); //Einlesen der Länge
	
	if 	(strcmp(argv[2],"mm")==0) ;
	else if (strcmp(argv[2],"cm")==0) l_ges=l_ges*10;
	else if (strcmp(argv[2],"m")==0) l_ges=l_ges*1000;
	else{
		 printf("Ungueltige Einheit\n");
		return 1;
	}	
	
	//VORBERECHNUNG


	int t_anfahrt = 2000; //Zeit zum Anfahren in ms
	int t_mitte = -1; // Zeit, die während der konstanten Periode vergeht in ms
	int t_step; //Zeit die für jeden Anfahr-Schritt benötigt wird in ms
	int l_anfahrt; //Länge, die bei der Anfahrt zurückgelegt wird in mm
	int l_mitte; //Länge, die während der konstanten Periode zurückgelegt wird in mm
	while (t_mitte<0){
	t_step = (int)(t_anfahrt/7); 
	//l_anfahrt = SUMME(f_step*t_step*U) = SUMME(f_step)*t_step*U
	//SUMME(f_step)= 10/50200ms + 10/28600ms + 10/19600ms + 10/14900ms + 10/14100ms + 10/13300ms + 10/13100ms + 10/13000ms = 0.004723888
	l_anfahrt = (int)(0.004723888*t_step*U); 
	l_mitte = (int)(l_ges - l_anfahrt*2); 
	t_mitte = l_mitte / (0.000769231*U); 
	if (t_mitte < 0) t_anfahrt -= 100;
	}


	//ÜBERPRÜFUNG	

	/*printf("d = %d mm\n",d);
	printf("U = %d mm\n",U);
	printf("l_ges = %d mm\n",l_ges);
	printf("t_anfahrt = %d ms\n",t_anfahrt);
	printf("t_step = %d ms\n",t_step);
	printf("t_mitte = %d ms\n",t_mitte);
	printf("l_anfahrt = %d mm\n",l_anfahrt);
	printf("l_mitte = %d mm\n",l_mitte); */

	//AUSFÜHRUNG
	//Der Motor wird in 7 50-er Schritten auf die Maximalgeschwindigkeit gesetzt, solange, bis die gewünschte Länge eingeholt ist.
	int dc=1500;
	unsigned long int zeit;

	//ANFAHREN
	printf("<ANFAHREN> [%d ms]\n",t_anfahrt);
	for (dc = 1500; dc>=1100;dc-=50)
	{
		servo_set(&servo, dc);
		zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
		while (xtimer_now_usec() < (zeit+t_step*1000));
		//printf("%d\n",dc);
	}

	//KONST. GESCHWINDIGKEIT
	printf("<MAXIMALGESCHWINDIGKEIT> [%d ms]\n",t_mitte);
	zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
	while (xtimer_now_usec() < (zeit+t_mitte*1000));

	//BREMSEN
	printf("<BREMSEN> [%d ms]\n",t_anfahrt);
	for (dc = 1100; dc<=1500;dc+=50)
	{
		servo_set(&servo, dc);
		zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
		while (xtimer_now_usec() < (zeit+t_step*1000));
		//printf("%d \n",dc);
	}	
	return 0;

}


static const shell_command_t shell_commands[] = { //Liste der Shell-Befehle
    { "set", "set servo value", _set },
    {"+", "Länge einholen", _geschw_neu},
    {"kalibrieren" , "Stellt den Motor auf einen Standartwert, der mithilfe des Potis als Nullpunkt eingestellt werden muss.", _kalibrieren},
    { NULL, NULL, NULL }
};






int main(void)
{
    int res;
    U =(int)(d*3.14159265358979323846); //Umfang der Spindel in mm

    /*puts("\nMotorsteuerung des RIOT-Projekts.");
    puts("Verbinden Sie den PWM-Eingang des Motors mit dem sechsten Pin von unten auf der rechten Seite");*/

    res = servo_init(&servo, DEV, CHANNEL, SERVO_MIN, SERVO_MAX);
    if (res < 0) {
        puts("Errors while initializing servo");
        return -1;
    }
    //puts("Servo initialized.");

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
