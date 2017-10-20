/* Rectangular signal to control servo at parallelport / 2.6
 demonstrates the effect of one-shot mode to exactly match
 the specified time*/
#include <vxWorks.h>
#include <stdio.h>
#include <time.h>
#include <taskLib.h>
#include <sysLib.h>
#include <iv.h>

#define STACK_SIZE  20000
#define TICKS 200  /* entspricht 20000 20kHz*/
#define LPT_PORT 0x378

#define ON		0
#define OFF		1

#define GATE1	0
#define GATE2	1
#define GATE3	2
#define GATE4	3
#define BLOWER	4
#define MOTOR	5

#define ALL_OFF				0xFF
#define MOTOR_WITH_BJ		0x0F
#define MOTOR_WITHOUT_BJ	0x1F

struct ball {
	int position; /* Between 0 and 11*/
	char color;
};

/* externe Funktionen   */
int init_428(void);
void dgau_428(char value);
char dgei_428(void);
void anau_428(char chn, float value);
float anei_428(char chn);
int SetBit_428(char chn1, char chn2);

int tidmytask, on_time, test;

int motorControlTask;
int blowerTask;

struct timespec start, stop;

double awert, elapsed;
char dout;

const int semOpt = 0;

int kugelDaFlag = 0;

char output = 0xFF;

int end = 0;

/* Blue */
float blue_R_max = 0.53;
float blue_R_min = 0.12;
float blue_G_max = 0.79;
float blue_G_min = 0.27;
float blue_B_max = 1.17;
float blue_B_min = 0.31;
float blue_I_max = 0.93;
float blue_I_min = 0.31;

/* Red */
float red_R_max = 1.37;
float red_R_min = 0.62;
float red_G_max = 0.73;
float red_G_min = 0.29;
float red_B_max = 0.34;
float red_B_min = 0.12;
float red_I_max = 0.87;
float red_I_min = 0.41;

/* Green */
float green_R_max = 0.15;
float green_R_min = 0.02;
float green_G_max = 0.43;
float green_G_min = 0.15;
float green_B_max = 0.21;
float green_B_min = 0.06;
float green_I_max = 0.33;
float green_I_min = 0.14;

/* Yellow */
float yellow_R_max = 2.09;
float yellow_R_min = 1.31;
float yellow_G_max = 1.94;
float yellow_G_min = 1.05;
float yellow_B_max = 0.62;
float yellow_B_min = 0.28;
float yellow_I_max = 1.51;
float yellow_I_min = 0.92;

int OurSetBit(char bit, char value) {
	int err_SetBit;
	/*printf("The value is: %02x\n", output);*/

	output ^= (-value ^ bit) & (1 << bit);
	dgau_428(output);

	/*printf("The changed value is: %02x\n", output);*/
	return (err_SetBit);
}

int off(void) {
	output = ALL_OFF;
	dgau_428(output);
	return 0;
}

int on(void) {
	output = 0x00;
	dgau_428(output);
	return 0;
}

void bj(void) {

	int i = 0;
	while (i < 250000) {
		i++;
		dgau_428(MOTOR_WITH_BJ);
	}

	dgau_428(MOTOR_WITHOUT_BJ);

}

void shiftSlots(char* slots, int size) {
	int x = size - 1;

	while (x > 0) {
		slots[x] = slots[x - 1];
		x--;
	}

	slots[0] = '.';
}

void ss(void) {

	/* taskDelete(tidmytask); */
	taskDelete(motorControlTask);
	taskDelete(blowerTask);

	output = ALL_OFF;
	dgau_428(output);
	

	printf("elapsed Task1= %6.1f\n", elapsed);
	printf("\n.....Task stop\n!");
}

void blower(void) {
	int amountOfBlows = 0;
	char actualValue;
	int blowerOn = 0;
	int endCounter = 0;
	while (endCounter < 3) {
		if (end != 0) {
			endCounter++;
		} else {
			endCounter = 0;
		}
		if (blowerOn != 0) {
			OurSetBit(BLOWER, OFF);
			blowerOn = 0;
			
			taskDelay(200);
		}
		actualValue = dgei_428();
		if ((actualValue & 0x40) > 0) {
		} else {
			if (blowerOn == 0) {
				OurSetBit(BLOWER, ON);
				amountOfBlows++;
				blowerOn = 1;
			}
		}
	}
	off();
}

int checkForEnd(char* slots, int size) {
	int i = 0;
	while (i < size) {
		if(slots[i] != '.') {
			return 0;
		}
		i++;
	}
	return 1;
}

void motorControl(void) {
	output = MOTOR_WITHOUT_BJ;
	dgau_428(output);
	char actualValue;
	char oldValue;
	int counter = 0;
	int initialized = 0;
	char oldBall = 0;
	char actualBall = 0;
	/*Checks if a slot is taken and by which color*/
	char slots[] =
			{ '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.'};
	while (1) {

		if (initialized == 0) {
			float r = anei_428(0);
			float g = anei_428(1);
			float b = anei_428(2);
			float i = anei_428(3);
			/*IMPORTANT: Red = 1, Green = 2, Yellow = 3, Blue = 4*/
			if (r <= red_R_max && r >= red_R_min && g <= red_G_max
					&& g >= red_G_min && b <= red_B_max && b >= red_B_min
					&& i <= red_I_max && i >= red_I_min) {
				actualBall = 'r';
			} else if (r <= blue_R_max && r >= blue_R_min && g <= blue_G_max
					&& g >= blue_G_min && b <= blue_B_max && b >= blue_B_min
					&& i <= blue_I_max && i >= blue_I_min) {
				actualBall = 'b';
			} else if (r <= yellow_R_max && r >= yellow_R_min
					&& g <= yellow_G_max && g >= yellow_G_min
					&& b <= yellow_B_max && b >= yellow_B_min
					&& i <= yellow_I_max && i >= yellow_I_min) {
				actualBall = 'y';
			} else if (r <= green_R_max && r >= green_R_min && g <= green_G_max
					&& g >= green_G_min && b <= green_B_max && b >= green_B_min
					&& i <= green_I_max && i >= green_I_min) {
				actualBall = 'g';
			} else {
				actualBall = '.';
			}

			if (oldBall != 0 && actualBall != 0 && oldBall != actualBall) {
				initialized = 1;
			}

			oldBall = actualBall;
		} else {
			actualValue = dgei_428();

			if (actualValue > oldValue) {
				counter = counter % 12;
				shiftSlots(slots, 12);
				end = checkForEnd(slots, 12);
				if (counter % 3 == 0) {
					float r = anei_428(0);
					float g = anei_428(1);
					float b = anei_428(2);
					float i = anei_428(3);
					/*IMPORTANT: Red = 1, Green = 2, Yellow = 3, Blue = 4*/
					if (r <= red_R_max && r >= red_R_min && g <= red_G_max
							&& g >= red_G_min && b <= red_B_max
							&& b >= red_B_min && i <= red_I_max
							&& i >= red_I_min) {
						slots[0] = 'r';
					} else if (r <= blue_R_max && r >= blue_R_min
							&& g <= blue_G_max && g >= blue_G_min
							&& b <= blue_B_max && b >= blue_B_min
							&& i <= blue_I_max && i >= blue_I_min) {
						slots[0] = 'b';
					} else if (r <= yellow_R_max && r >= yellow_R_min
							&& g <= yellow_G_max && g >= yellow_G_min
							&& b <= yellow_B_max && b >= yellow_B_min
							&& i <= yellow_I_max && i >= yellow_I_min) {
						slots[0] = 'y';
					} else if (r <= green_R_max && r >= green_R_min
							&& g <= green_G_max && g >= green_G_min
							&& b <= green_B_max && b >= green_B_min
							&& i <= green_I_max && i >= green_I_min) {
						slots[0] = 'g';
					} else {
						slots[0] = '.';
					}
				} else {
					slots[0] = '.';
				}
				 
				counter++;
			}

			if (slots[0] == 'r') {
				SetBit_428(GATE1, ON);
				/*printf("\t%d\n",counter);*/
			} else {
				SetBit_428(GATE1, OFF);
			}
			if (slots[2] == 'g') {
				SetBit_428(GATE2, ON);
				/*printf("\t%d\n",counter);*/
			} else {
				SetBit_428(GATE2, OFF);

			}
			if (slots[5] == 'y') {
				SetBit_428(GATE3, ON);
				/*printf("\t%d\n",counter);*/
			} else {
				SetBit_428(GATE3, OFF);

			}
			if (slots[7] == 'b') {
				SetBit_428(GATE4, ON);
				/*printf("\t%d\n",counter);*/
			} else {
				SetBit_428(GATE4, OFF);

			}

			oldValue = actualValue;
		}
		taskDelay(5);

	}
}

int rot(void) {
	int i = 0;

	while (i < 500000) {
		i++;
		output = MOTOR_WITHOUT_BJ;
		dgau_428(output);
	}
	i = 0;
	output = ALL_OFF;
	dgau_428(output);

	return 0;
}

int sr(void) {

	/*tasks erstellen*/

	/*farbeErkennenTaskVar = taskSpawn ("farbeErkennenTask", 40,0, STACK_SIZE, (FUNCPTR)farbeErkennenTask, 0,0,0,0,0,0,0,0,0,0); */

	motorControlTask = taskSpawn("motorControl", 11, 0, STACK_SIZE,
			(FUNCPTR) motorControl, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	blowerTask = taskSpawn("blower", 11, 0, STACK_SIZE, (FUNCPTR) blower, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0);

	init_428();
	output = ALL_OFF;
	dgau_428(output);
	

	sysClkRateSet(TICKS);
	test = sysClkRateGet();

	printf("Systemclock ticks per second =%i \n", test);
	printf("Task start!\n");

	return 0;

}



