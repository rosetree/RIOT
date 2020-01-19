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

#include "shell.h"

#define DEV         PWM_DEV(0) //PWM-Device
#define CHANNEL     2 //PWM-Kanal
#define SERVO_MIN	(500U) //Mindestwert des Servos. Spielt keine große Rolle, wird aber als Eingabewert gefordert.
#define SERVO_MAX 	(5500U) //Maximalwert des Servos. Spielt keine große Rolle, wird aber als Eingabewert gefordert.

#define PI 3.14159265358979323846


#define STILLSTAND	(1500U) //Bei dieser Zeit steht der Motor still. Sollte dies nicht der Fall sein muss er kalibriert werden.
#define MAXGESCHW	(1300U) //Bei dieser Zeit fährt der Motor auf voller Geschwindigkeit vorwärts
#define MINGESCHW	(1700U) //Bei dieser Zeit fährt der Motor auf voller Geschwindigkeit rückwärts

//#define MINGESCHW	(2680U)





servo_t servo;



int d=15; //Durchmesser der Spindel in mm, wird auf 15 mm initialisiert
int U=57; //Umfang der Spindel in mm, wird auf 57 mm initialisiert



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


static int _set_d(int argc, char**argv)
{
	if (argc != 3) {
		printf("usage: %s <value> <unit>\n", argv[0]);
	        return 1;
	}

	if 	(strcmp(argv[2],"mm")==0) d = atoi(argv[1]);
	else if (strcmp(argv[2],"cm")==0) d = atoi(argv[1])*10;
	else if (strcmp(argv[2],"m")==0)  d = atoi(argv[1])*1000;
	else{
		 printf("Ungueltige Einheit\n");
		 return 1;
	}	
	U =(int)(d*PI);
	return 0;
}



static int _set(int argc, char**argv) //setzt den DutyCicle-Wert des Motors auf einen Wert in Mikrosekunden
{
    if (argc != 2) {
        printf("usage: %s <value> \n", argv[0]);
        return 1;
    }


    servo_set(&servo, atoi(argv[1]));
    printf("auf %u gesetzt \n",atoi(argv[1]));
    return 0;
}

uint32_t winch_measure (winch_t *winch, uint32_t *messwerte) //Diese Funktion ist bis zum Ende des Semesters nicht fertig geworden, und erfüllt noch keinen sinnvollen Zweck
//Führt Messungen der Zeiten durch, die die Winde bei den verschiedenen Anfahrgeschwindigkeiten für eine Umdrehung braucht
{
	int dc=1500;
	uint32_t zeit;
	int i=0; 

	//Servo in Startposition bringen	
	if (gpio_read(winch->reed)!=0){
		while(gpio_read(winch->reed)!=0)
		{
			printf("xxx\n");
			servo_set(&servo, 1200);
		}	
	}
	printf("2\n");
	while(gpio_read(winch->reed)==0)
	{
		servo_set(&servo, 1200);
	}
	servo_set(&servo, 1500);		



	for (dc = 1450; dc>=1100;dc-=50)
	{
		servo_set(&servo, dc);		
		zeit=xtimer_now_usec(); 
		while(gpio_read(winch->reed)!=0);
		while(gpio_read(winch->reed)==0);
		messwerte[i]=xtimer_now_usec()-zeit;
		printf("Messwert %d:  dc= %d;     %"PRIu32"\n",i,dc,messwerte[i]);
		i++;
	}	
	servo_set(&servo, 1150);
	servo_set(&servo, 1200);
	servo_set(&servo, 1250);
	servo_set(&servo, 1300);
	servo_set(&servo, 1350);
	servo_set(&servo, 1400);
	servo_set(&servo, 1450);
	servo_set(&servo, 1500);
	return 1/messwerte[0]+1/messwerte[1]+1/messwerte[2]+1/messwerte[3]+1/messwerte[4]+1/messwerte[5]+1/messwerte[6]+1/messwerte[7];

return 0;
}


int winch_init (winch_t *winch, servo_t *servo, int d, gpio_t reed)
{
	winch->servo = servo; //Das Servomotor-Objekt, welches gesteuert werden soll
	winch->d = d; //Durchmesser der Spindel
	winch->U = d * PI; //Umfang der Spindel
	winch->reed = reed; //Reed-Switch zur Messung, ist in aktueller Fassung noch nicht programmiert.
	return 0;
}


void winch_control (winch_t *winch, int l_ges 
//Steuert die Bewegungen des Motors. Es wird ein Wert in mm eingegeben, der je nach Vorzeichen den Motor vor oder zurück dreht.
//Die Bewegung folgt mit einer Anfahr- und einer Bremsphase
{
	int t_anfahrt = 2000; //Zeit zum Anfahren in ms
	int t_mitte = -1; // Zeit, die während der konstanten Periode vergeht in ms
	int t_step; //Zeit die für jeden Anfahr-Schritt benötigt wird in ms
	int l_anfahrt; //Länge, die bei der Anfahrt zurückgelegt wird in mm
	int l_mitte; //Länge, die während der konstanten Periode zurückgelegt wird in mm
	while (t_mitte<0) //Die Anfahrtszeit wird zunächst auf 2 s festgelegt. sollte dies zu negativen Zeiten führen, wird sie schrittweise herabgesetzt.
	{
		t_step = (int)(t_anfahrt/7); 
		//l_anfahrt = SUMME(f_step*t_step*U) = SUMME(f_step)*t_step*U
		//SUMME(f_step)= 10/50200ms + 10/28600ms + 10/19600ms + 10/14900ms + 10/14100ms + 10/13300ms + 10/13100ms + 10/13000ms = 0.004723888
		l_anfahrt = (int)(0.004723888*t_step* winch->U); //Die errechneten Werte beziehen sich auf Messungen der Umlaufzeiten 
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
			servo_set(&servo, dc);
			zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
			while (xtimer_now_usec() < (zeit+t_step*1000));
		}

		zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
		while (xtimer_now_usec() < (zeit+t_mitte*1000));

		for (dc = 1100; dc<=1500;dc+=50)
		{
			servo_set(&servo, dc);
			zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
			while (xtimer_now_usec() < (zeit+t_step*1000));
		}		

	}

	else //negative direction
	{
		for (dc = 1500; dc<=1900;dc+=50)
		{
			servo_set(&servo, dc);
			zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
			while (xtimer_now_usec() < (zeit-t_step*1000));
		}

		zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
		while (xtimer_now_usec() < (zeit-t_mitte*1000));

		for (dc = 1900; dc>=1500;dc-=50)
		{
			servo_set(&servo, dc);
			zeit=xtimer_now_usec(); //aktuelle Zeit wird gespeichert
			while (xtimer_now_usec() < (zeit-t_step*1000));
		}		
	}


}


static int _plus(int argc, char**argv)
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
	winch_control(&winch, l_ges);
	return 0;



}


static int _minus(int argc, char**argv)
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
	
	winch_control(&winch,-1*l_ges);
	return 0;
}



static const shell_command_t shell_commands[] = { //Liste der Shell-Befehle
    { "set", "set servo value", _set },
    {"+", "Länge ausgeben", _plus},
    {"-", "Länge einholen", _minus},
    {"set_d", "Durchmesser der Spindel eingeben", _set_d},
    {"kalibrieren" , "Stellt den Motor auf einen Standartwert, der mithilfe des Potis als Nullpunkt eingestellt werden muss.", _kalibrieren},
    { NULL, NULL, NULL }
};






int main(void)
{ 
    gpio_t reed_pin= GPIO_PIN(0,23);
    int d = 15; //Spindeldurchmesser in mm
    gpio_init(reed_pin,GPIO_IN_PD);
    //uint32_t messwerte[10]; gg

    int res = servo_init(&servo, DEV, CHANNEL, SERVO_MIN, SERVO_MAX);
    if (res < 0) {
        puts("Errors while initializing servo");
        return -1;
    }
	//while(1) printf("%d\n",gpio_read(reed_pin));


    winch_init(&winch,&servo,d, reed_pin);

    char line_buf[SHELL_DEFAULT_BUFSIZE];

    // winch_measure (&winch, messwerte); 


    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

	

    return 0;
}
