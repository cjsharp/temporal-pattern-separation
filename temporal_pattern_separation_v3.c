	//choose modes
#define  punish_onoff  0 	//set to 1 to delay mouse if it doesn't lick correctly or at all; set 0 otherwise
#define  blocks  1     		//set to 1 if you want to use the block training protocol; set to 0 otherwise
#define  training  1		//set to 1 if you want the appropriate reward valve to open after every sequence of tones regadrdless of mouse lick
	//configurable parameters
#define  duration_punishment  (5)/.02            	//set time you want the punishment to be in seconds in the parenthesises
#define  duration_delay  (3)/.02 
#define  duration_tones  (unsigned int)((.2)/.02)          		//set duration of tones in seconds in the parenthesises
#define  duration_time_between_tones  (unsigned int)((.3)/.02)	//set duration of time between tones in the parenthesises
#define  duration_lickwindow  (2)/.02        	//set duration of lickwindow in seconds in the parenthesises
#define  duration_open_valve  (.1)/.02 		  	//set duration of open reward valve in seconds
#define  trial_number  200       				//sets number of trials
//************************************************************************************************************************************
#define  duration_tone_and_space  duration_tones+duration_time_between_tones
#define  duration_song  4*duration_tones+3*duration_time_between_tones

//implement 12T mode and fix timings
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
#define FOSC 24000000L 		   //quartz crystal oscillator frequency
#define BAUD 9600			   //baurdrate
#define a (256-FOSC/2/7040/12) //7.04KHz frequency calculation method of 12T mode
#define b (256-FOSC/2/7902/12) //7902Hz
#define c (256-FOSC/2/8372/12) //8372Hz 
#define d (256-FOSC/2/9397/12) //9397Hz
#define e (256-FOSC/2/8372/12) //8372Hz
#define f (256-FOSC/2/11175/12)//1175Hz
#define g (256-FOSC/2/12544/12)//12544Hz 

bit busy;					//bit used in uart interrupt
bit target;					//target bit used to tell if song is target=1/nontarget=0
bit tonechange;				//when tone is changed in timer0, tonechange goes HIGH so that main function knows to switch BRT/tone frequency
bit lickflag;				//lickflag goes HIGH in external ISRs. lets main function to know when to check value of correct variable
bit dripflag;				//once lick is checked for correctness in main function, lickflag goes 0, dripflag goes 1. timer0 knows
unsigned char k=0;			//index used for tones			    it can switch to next phase when dripflag = 1 and resets it to zero
unsigned char correct;				//correct=2 => mouse licked correctly;    correct=1 => incorrect;    correct=0 => no lick
unsigned int continuouscounter = 0; //I keep this timer0 overflow counter in the timer0 isr so i can calculate precise timings for dropfunction
unsigned char phasecounter = 0;      //+1 everytime timer0 overflows, resets in timer0 when it's time for phase to change
unsigned int phase = 0;    			//phase = 0 song; phase = 1 lickwindow; phase = 2 delay
sbit BRTCLKO = P1^0;
sbit leftvalve = P1^1;
sbit rightvalve = P1^2;	

//void SendChar(unsigned char x){
//	//transmit byte,wait for byte to be transmitted, clear transmit interrupt flag
//	SBUF=0x01;while(TI==0){};TI=0;		 //this first line is to distinguish it from scheduled transmissions
//	SBUF=x;while(TI==0){};TI=0;  		 //this line transmits the actual info
//	SBUF='\n';while(TI==0){};TI=0;}		 //this line transmits newline	   	  
 
void timer0(void) interrupt 1 {
	TL0=0xBF;            //initial values loaded to timer 50HZ (.02s period) 
	TH0=0x63;            //65355-(24,000,000/12/50)=25355 25355(dec)->63BF(hex) TH0=63 TL0=BF	
    continuouscounter++; //used for calculating times for dripfunction 
	phasecounter++;		 //phasecounter resets every phase change	
	if (phase==0){		 //song
		if (phasecounter == (unsigned int)k*duration_tone_and_space){k++; tonechange=1; WAKE_CLKO=0x04;}  //change tone, turn on tone
		else if (phasecounter == ((unsigned int)k*duration_tone_and_space)-duration_time_between_tones){WAKE_CLKO=0;}   //turn off tone
		else if (phasecounter == duration_song){phase=1; WAKE_CLKO=0; phasecounter=0; k=0;}  //change phase, turn off tone,
	}					 //lickwindow												//reset phasecounter, reset tone index
	else if (phase==1){
		if (dripflag){phase=2; dripflag=0; phasecounter=0;}//after main()check to see if mouse's lick is correct, change phase, reset phasecounter, reset dripflag 
		else if (phasecounter==duration_lickwindow){correct=0; phase=2; phasecounter=0;}//if no lick after 2 sec, change phase, set correct=0, reset phasecounter 
	}					 //delay period
	else if (phase == 2){		 //if punish_onoff==1 & mouse is incorrect, punish by making it wait extra long b4 next song
		if (punish_onoff==1 & correct!=2 & phasecounter >= duration_delay+duration_punishment){phase=0; phasecounter=0;}
		else if (phasecounter >= duration_delay){phase=0; phasecounter=0;}	 //else mouse just waits regular time for next song
	}
}

void exint0() interrupt 0 {	   //left lick interrupt (location at 0003H)
	printf("bl\n");
	if (phase==1){lickflag=1;}//turn on lickflag if lickwindow is one, tells main function it can check if correct=0,1,2  
	if (target){correct = 1;}//if mouse was supposed to lick right lead, it is incorrect, so correct =1
	else {correct = 2;}}//if mouse was supposed to lick left lead, it lick correctly, correct = 2

void exint1() interrupt 2 {	   //right lick interrupt (location at 0013H)
	printf("br\n");    	
	if (phase==1){lickflag=1;}//turn on lickflag if lickwindow is one, tells main function it can check if correct=0,1,2
	if (target){correct = 2;}//if mouse was supposed to lick right lead, it is incorrect, so correct=2
	else {correct = 1;}}//if mouse was supposed to lick left lead, it lick correctly, correct = 1

void Uart_Isr() interrupt 4 {
    if (RI){RI = 0; P0 = SBUF;}    	//Clear receive interrupt flag; P0 show UART data
    if (TI){TI = 0; busy = 0;}}    	//Clear transmit interrupt flag; Clear transmit busy flag 
	                 
void dripfunction(bit targetz){
	unsigned int initialtime = 156*continuouscounter+(unsigned int)TH0-99; 
	unsigned int finaltime = 156*continuouscounter+(unsigned int)TH0-99;
	if (targetz == 0){leftvalve = 1;}
	else {rightvalve = 1;}
	while (finaltime-initialtime <= (unsigned int)(.02*duration_open_valve/156)){
		finaltime = 156*continuouscounter+((unsigned int)TH0-99);
	}
	rightvalve=0; leftvalve=0;} 

void main(){
	bit generatesequence; 		//goes HIGH before delay loop, song of tones is generated during delay, then goes LOW again
	unsigned char i=0;			//used to index tones when generating sequence
	unsigned char sequence[4];
	unsigned int j=0; 			//j is the index used to indicate which sequence is being played 		
	unsigned char code toneslist[7] = {0,1,2,3,4,5,6}; //array used to randomly generate sequences {a,b,c,d,e,f,g}
//	P1 = 0x00;
//	P3 = 0x0f;
	SCON = 0x5a;                    //8 bit data, no parity bit
	TMOD = 0x21; 	                //8-bit auto reload timer 1 and 16 bit timer 0
	IE = 0x82;
//	IE = 0x9F;    				//enable global interrupt, and serial, both timer, and both external interrupts 
	TH1 = TL1 = 0xFD;               //Set Uart baudrate	(9600?)
	TL0=0xBF;                       //initial values loaded to timer 50HZ (.02s period) 
	TH0=0x63;                       //65355-(24,000,000/12/50)=25355 25355(dec)->630B(hex) TH1=63 TL1=0B
	TCON= 0x50;						//start timers 0 and 1
	IT1 = 1;                        //set INT1 int type (1:Falling only 0:Low level)   not sure if these lines are necessary
    EX1 = 1;                        //enable INT1 interrupt	
	IT0 = 1;                        //set INT0 int type (1:Falling 0:Low level)
    EX0 = 1;                        //enable INT0 interrupt
	WAKE_CLKO = 0;					//no tone generation
	srand(7);//need random seed?
	target = 0;	
	while(j<=trial_number){
		generatesequence = 1;			
		do{									     		//first time through the delay: we need to generate song. after delay, 
			if (generatesequence){								    	  //we idly wait for timer0 to change phase to move on
				if (blocks & j%3 == 0){target = !target;}		//phase2 starts by deciding whether song is target or nontarget
				else if (correct == 2){target = rand()%2;}			//then generates the appropriate target/nontarget sequence
				if (target){sequence[0]=2; sequence[1]=3; sequence[2]=4; sequence[3]=5;} 	//if target==1, sequence = cdef
				else {for (i = 0 ; i<= 3 ; i++){sequence[i] = toneslist[rand()%7];}}	//if not, random sequence
				generatesequence=0;}				//set generatesequence back to zero so that sequence is generated only once
		}while(phase==2);	   						//in the delay. loop does nothing after song is composed until delay is over					
//		printf("j=%u\n",j);
		j++;										//marks jth trial/song that the mouse listens to
		while(phase==0){							//song starts here
			printf("phase0counter=%u\n",(unsigned int)phasecounter);
			if (tonechange){			//if timer0 adds 1 to the tone index, k, tonechange is set HIGH which indicates 
				tonechange = 0;			//that it needs to switch to next tone checks which tone is supposed to play															
				switch (sequence[k]) {	//and sets BRT to the value such that it produces the appropriate frequency
					case 0: BRT = a; break; case 1: BRT = b; break; case 2: BRT = c; break;
					case 3: BRT = d; break; case 4: BRT = e; break; case 5: BRT = f; break;
					case 6: BRT = g; break;
				}
//				printf("tone=%x\n",(unsigned int)sequence[k]);
			}
		}											//phase 1 starts by printing out tone sequence 
		printf("sequence=%04x\n",(((sequence[0]<<4|sequence[1])<<4)|sequence[2]<<4)|sequence[3]);
		if (training == 1){dripfunction(target);}	//if we're training mice, give them fluids at beginning of lickwindow no matter what
		while(phase == 1){							//starts checking for mouse licks in loop
			if (lickflag==1 & correct==2 & training==0){dripfunction(target); lickflag=0; dripflag=1;}//if mouse licks correctly,
			else if (lickflag==1 & correct==1 & training==0){lickflag=0; dripflag=1;}//reward. if not, no reward. switch flags so
		}																		//timer knows it can set phase=2 and reset counter
		printf("sequencedata=%x\n",(unsigned int)(correct<<6|target)); //print correct and target variables so we know what mouse was supposed lick and if it was correct 
	}
	phase = 3;//i should disable timers here
	while(1){printf("end\n");}
}

 
