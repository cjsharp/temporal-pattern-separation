	//choose modes
#define  delay_onoff  0 	//set to 1 to delay mouse if it doesn't lick correctly or at all; set 0 otherwise
#define  blocks  0     		//set to 1 if you want to use the block training protocol; set to 0 otherwise
#define  training  1		//set to 1 if you want the appropriate reward valve to open after every sequence of tones regadrdless of mouse lick
	//configurable parameters
#define  duration_delay  (5)/.02            	//set time you want the delay to be in seconds in the parenthesises
#define  duration_tones  (unsigned int)((.2)/.02)          		//set duration of tones in seconds in the parenthesises
#define  duration_time_between_tones  (unsigned int)((.3)/.02)	//set duration of time between tones in the parenthesises
#define  duration_lickwindow  (2)/.02        	//set duration of lickwindow in seconds in the parenthesises
#define  duration_open_valve  .1	 		  	//set duration of open reward valve in seconds
#define  trial_number  200       				//sets number of trials
//************************************************************************************************************************************

//implement minumum difficulty
//check timers/BRG with oscope
//check logic with arduino
//check dripfunction

//later: embed serial graphs into GUI

#include <reg60s2.h>
#include <stdio.h>
#include <intrins.h>
#include <math.h>
#include <stdlib.h>
#define FOSC 24000000L 
#define BAUD 9600
#define a (256-FOSC/2/7040) //7.04KHz frequency calculation method of 1T mode
#define b (256-FOSC/2/7902) //7902Hz
#define c (256-FOSC/2/8372) //8372Hz 
#define d (256-FOSC/2/9397) //9397Hz
#define e (256-FOSC/2/8372) //8372Hz
#define f (256-FOSC/2/11175)//1175Hz
#define g (256-FOSC/2/12544)//12544Hz 

bit busy;
unsigned int continuouscounter = 0;		//counts timer 0 overflows every .02s, resets to 0 after 50
unsigned int seconds = 0;				//+1 every time continuouscounter >= 50
unsigned int phasecounter = 0;          //+1 everytime timer0 overflows, reset by main function when phase changes
unsigned int realtimelick;
unsigned int phase = 0;    /*phase = 0 means indicates that the tones are playing; phase = 1 indicates that 
      the lickwindow is on; phase = 2 indictates that it is the delay time between lickwindow and next song*/
////////P1^0=0;
////////P1^1=0;
////////P1^2=0;
sbit BRTCLKO = P1^0;
sbit leftvalve = P1^1;
sbit rightvalve = P1^2;	

void timer0(void) interrupt 1 {   	//global timekeeper
	TL0=0xBF;                       // initial values loaded to timer 50HZ (.02s period) 
	TH0=0x63;                       // 65355-(24,000,000/12/50)=25355 25355(dec)->63BF(hex) TH0=63 TL0=BF
	phasecounter++;
	continuouscounter++;
	if (continuouscounter >= 50){
	  continuouscounter = 0;
	  seconds++; 
	}	
}

void exint0() interrupt 0 {     //left lick (location at 0003H)
	if (phase == 1){phase = 2;}
	if (realtimelick == 3){realtimelick=4;}
	else if (realtimelick == 5) {realtimelick=6;}}

void exint1() interrupt 2 {    	//right lick (location at 0013H)
	if (phase == 1){phase = 2;}
	if (realtimelick == 3){realtimelick=5;}
	else if (realtimelick == 4){realtimelick=6;}}

void Uart_Isr() interrupt 4 {
    if (RI){RI = 0; P0 = SBUF;}    	//Clear receive interrupt flag; P0 show UART data
    if (TI){TI = 0; busy = 0;}}    	//Clear transmit interrupt flag; Clear transmit busy flag                  

void dripfunction(unsigned int whatmouseshouldlickz){
	int initialtime = 200000*continuouscounter+5*(((unsigned int)TH0<<8|(unsigned int)TL0)-25355);
	int finaltime = 200000*continuouscounter+5*(((unsigned int)TH0<<8|(unsigned int)TL0)-25355);
	if (whatmouseshouldlickz == 16){
		leftvalve = 1;	
	}
	else if  (whatmouseshouldlickz == 18){
		rightvalve = 1;
	}
	while (finaltime-initialtime <= duration_open_valve){ 
		printf("%u.%07lu,%u,%u\n",seconds,200000*continuouscounter+5*(long unsigned int)(((unsigned int)TH0<<8|(unsigned int)TL0)-25355),phase,realtimelick);
		realtimelick = 3;
		rightvalve = 0;
		leftvalve = 0;
		finaltime = 200000*continuouscounter+5*(((unsigned int)TH0<<8|(unsigned int)TL0)-25355);
	}
} 

void main(){ 
	char xdata sequence[200][4];
	unsigned int  j; 					//j is the index used to indicate which sequence is being played
	unsigned int  k;					//k is the index used to indicate which tone is playing of the trial_numberth sequence
	unsigned int whatmouselicks;
	unsigned int whatmouseshouldlick;        
	unsigned char correct = 0;			//correct=1 indicates mouse licked correctly; correct = 0 incorrect lick; correct = -1 indicates no lick
	bit target = 0;			//if target = 0, target sequence plays. if target = 1, nontarget sequence plays
	unsigned char xdata block_sequence[500];
	unsigned char code toneslist[8] = {'a','b','c','d','e','f','g','\0'};		//array used to randomly generate sequences
	unsigned int duration_tones_and_space = duration_tones + duration_time_between_tones;
	unsigned int duration_sequence = 4*duration_tones + 3*duration_time_between_tones;
//	P1 = 0x00;
	P3 = 0x0f;
	SCON = 0x5a;                    //8 bit data, no parity bit
	TMOD = 0x21; 	                //8-bit auto reload timer 1 and 16 bit timer 0
	IE = 0x82;
//	IE = 0x9F;    				//enable global interrupt, and serial, both timer, and both external interrupts 
	TH1 = TL1 = 0xFD;               //Set Uart baudrate	(9600?)
	TL0=0x0B;                       //initial values loaded to timer 50HZ (.02s period) 
	TH0=0x63;                       //65355-(24,000,000/12/50)=25355 25355(dec)->630B(hex) TH1=63 TL1=0B
	TCON= 0x50;						//start timers 0 and 1
	IT1 = 1;                        //set INT1 int type (1:Falling only 0:Low level)   not sure if these lines are necessary
    EX1 = 1;                        //enable INT1 interrupt	
	IT0 = 1;                        //set INT0 int type (1:Falling 0:Low level)
    EX0 = 1;                        //enable INT0 interrupt
	WAKE_CLKO = 0;			
	for (j = 0 ; j <= trial_number ; j++){						
		for (k = 0 ; k <= 3 ; k++){ //this loop generates nontarget sequence in the second column of sequence
			unsigned int randindex = rand() % 7;
			sequence[j][k] = toneslist[randindex];
	    }
//	  	printf("%c%c%c%c\n",sequence[j][0],sequence[j][1],sequence[j][2],sequence[j][3]);
	}
	for (j = 0 ; j <= trial_number ; j++){							
		if (j%3 == 0){
			target = !target;
		}
		block_sequence[j]=target;
	}
	srand(7);//need random seed?
	target = 0;	
	for(j = 0 ; j < trial_number ; j++){
		if (blocks){
			target = (int)block_sequence[j];
		}
		if (target){
			sequence[j][0] = 'c';        //if it's supposed to play target sequence
			sequence[j][1] = 'd';        //it overwrites the nontarget sequence that was
			sequence[j][2] = 'e';		 //that was there before
			sequence[j][3] = 'f';
		}
		whatmouseshouldlick = (unsigned int)target*2+14;
		k = 0;
		phasecounter = 0;			
		while(phase == 0){		  															//song loop
			if (phasecounter == k*duration_tones_and_space){
				switch (sequence[j][k]) {
					case 'a': BRT = a; break; case 'b': BRT = b; break; case 'c': BRT = c; break;
					case 'd': BRT = d; break; case 'e': BRT = e; break; case 'f': BRT = f; break;
					case 'g': BRT = g; break;
				}
				k++;
				WAKE_CLKO = 0x04;
			}
		    else if (phasecounter == (k*duration_tones_and_space)-duration_time_between_tones){
		    	WAKE_CLKO = 0;
			}
			else if (phasecounter >= duration_sequence){
				phase = 1;
    			WAKE_CLKO = 0;
			}
			printf("%u.%07lu,%u,%u\n",seconds,200000*continuouscounter+5*(long unsigned int)(((unsigned int)TH0<<8|(unsigned int)TL0)-25355),phase,realtimelick);
			realtimelick = 3;
		}
        phasecounter = 0;
		while (training == 1 && phase == 1){ 										//training lickwindow loop
			dripfunction(whatmouseshouldlick);
        	printf("%u.%07lu,%u,%u\n",seconds,200000*continuouscounter+5*(long unsigned int)(((unsigned int)TH0<<8|(unsigned int)TL0)-25355),phase,realtimelick);
			realtimelick = 3;
			if (phasecounter == duration_lickwindow){
				phase = 2;
			}
		}
		phasecounter = 0;
		while(training == 0 && phase == 1){	  										//regular lickwindow loop
			printf("%u.%07lu,%u,%u\n",seconds,200000*continuouscounter+5*(long unsigned int)(((unsigned int)TH0<<8|(unsigned int)TL0)-25355),phase,realtimelick);
			switch (realtimelick) {
				case 5: whatmouselicks = 18; break;
				case 4: whatmouselicks = 16; break;
				case 6: whatmouselicks = 20; break;
			}
			realtimelick = 3;
			if (whatmouselicks == whatmouseshouldlick){
				dripfunction(whatmouseshouldlick);
				target = rand()%2;
				phase = 0;
			}
			else{
				phase = 2;
			}			  
			if (phasecounter == duration_lickwindow){
				whatmouselicks = 14;
				phasecounter = 0;
				phase = 2;
			}
		}
		phasecounter=0;
		if ((int)delay_onoff){
			while (phase == 2){					  								//delay loop
				if (phasecounter == duration_delay){
					phase = 0;
				}
				printf("%u.%07lu,%u,%u\n",seconds,200000*continuouscounter+5*(long unsigned int)(((unsigned int)TH0<<8|(unsigned int)TL0)-25355),phase,realtimelick);
			realtimelick = 3;		
		  	}
		}
		phase = 0;
		printf(",,,%u,%u,%u,%c%c%c%c\n",j,whatmouselicks,whatmouseshouldlick,sequence[j][0],sequence[j][1],sequence[j][2],sequence[j][3]);
	}
	while(1){
		printf("end");
	}
}
