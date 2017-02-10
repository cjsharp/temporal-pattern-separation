	//choose modes
#define  punish_onoff  0 	//set to 1 to delay mouse if it doesn't lick correctly or at all; set 0 otherwise
#define  blocks  1     		//set to 1 if you want to use the block training protocol; set to 0 otherwise
#define  training  1		//set to 1 if you want the appropriate reward valve to open after every sequence of tones regadrdless of mouse lick
	//configurable parameters
#define  duration_punishment  (unsigned int)((5)/.01)          	//set time you want the punishment to be in seconds in the parenthesises
#define  duration_delay  (unsigned int)((3)/.01) 		 
#define  duration_tones  (unsigned int)((.2)/.01)          		//set duration of tones in seconds in the parenthesises
#define  duration_time_between_tones  (unsigned int)((.3)/.01)	//set duration of time between tones in the parenthesises
#define  duration_lickwindow  (2)/.01        					//set duration of lickwindow in seconds in the parenthesises
#define  duration_reward  (.1)/.01 		  						//set duration of open reward valve in seconds
#define  trial_number  200       								//sets number of trials
#define  minimum_difficulty 4
//************************************************************************************************************************************

//check external interrupts
//check timers/BRG with oscope
//check logic with arduino

//later: embed serial graphs into GUI

#include <reg60s2.h>
#include <stdio.h>
#include <intrins.h>
#include <math.h>
#include <stdlib.h>
#define FOSC 24000000L 		//quartz crystal oscillator frequency
#define BAUD 9600			//baurdrate
#define a (256-FOSC/2/7040) //7.04KHz frequency calculation method of 1T mode
#define b (256-FOSC/2/7902) //7902Hz
#define c (256-FOSC/2/8372) //8372Hz 
#define d (256-FOSC/2/9397) //9397Hz
#define e (256-FOSC/2/8372) //8372Hz
#define f (256-FOSC/2/11175)//1175Hz
#define g (256-FOSC/2/12544)//12544Hz 

bit busy;					//bit used in uart interrupt
bit target;					//target bit used to tell if song is target=1/nontarget=0
bit tonechange=0;			//when tone is changed in timer0, tonechange goes HIGH so that main function knows to switch BRT/tone frequency
bit lickflag=0;
unsigned int beginreward;
unsigned char k=0;				//index used for tones			    
unsigned char correct;			//correct=2 => mouse licked correctly;    correct=1 => incorrect;    correct=0 => no lick
unsigned int phasecounter = 0;	//+1 everytime timer0 overflows, resets in timer0 when it's time for phase to change
unsigned char phase = 0;    	//phase=0 song; phase=1 lickwindow; phase=2 reward; phase=3 delay
sbit BRTCLKO = P1^0;			//pin 1
sbit leftvalve = P1^1;			//pin 2
sbit rightvalve = P1^2;			//pin 3  	  
 
void timer0(void) interrupt 1 {
	TL0=0x2B;            //initial values loaded to timer 100HZ (.01s period)
	TH0=0xB1;            //65355-(24,000,000/12/100)=45355 45355(dec)->B12B(hex) TH0=B1 TL0=2B	
	phasecounter++;		 //phasecounter resets every phase change		
	if (phase==0){
		if (phasecounter == 0){k=0; tonechange=1; WAKE_CLKO=0x04;}
		else if (phasecounter == duration_tones){k++; tonechange=1; WAKE_CLKO=0;}  			//next tone, turn on tone
		else if (phasecounter == duration_tones+duration_time_between_tones){WAKE_CLKO=0x04;}
	  	else if (phasecounter == 2*duration_tones+duration_time_between_tones){k++; tonechange=1; WAKE_CLKO=0;}
		else if (phasecounter == 2*duration_tones+2*duration_time_between_tones){WAKE_CLKO=0x04;}
	   	else if (phasecounter == 3*duration_tones+2*duration_time_between_tones){k++; tonechange=1; WAKE_CLKO=0;}
	   	else if (phasecounter == 3*duration_tones+3*duration_time_between_tones){WAKE_CLKO=0x04;}
		else if (phasecounter == 4*duration_tones+3*duration_time_between_tones){ 							//song ends
			phasecounter=0; WAKE_CLKO=0;							//turn off tone; reset tone index; reset phasecounter 
			if(training){phase=2;									//go to reward phase if training when song ends
				if (target){rightvalve=1;} else {leftvalve=1;}}		//reward from left or right lead
			else{phase=1;}}}										//go to lickwindow if not training after song ends	
	if(phase==1){
		if (phasecounter == duration_lickwindow){			//if no lick after 2 sec in not training mode,
			phasecounter=0; correct=0; phase=3;}			//go to delay phase, set correct=0, phasecounter=0
		else if (lickflag){lickflag=0; 						//if mouse licks, phasecounter=0
			if (correct==2){phase=2;}						//go to reward phase if correct
			else if (correct==1){phase=3;}}}				//if not, go to delay phase  
	if (phase == 2){
		if (target){rightvalve=1;} else {leftvalve=1;}				//reward mouse from left or right lead	
		if(phasecounter == duration_reward){	  		
			rightvalve=0; leftvalve=0; phase=3; phasecounter=0;}}	//change phase, reset phasecounter, reset dripflag
	if (phase == 3){
		if (punish_onoff & correct!=2 & phasecounter==duration_punishment+duration_delay){phase=0; phasecounter=0;}
		else if (!punish_onoff & phasecounter==duration_delay){phase=0; phasecounter=0;}}}
	

void exint0() interrupt 0 {	   //left lick interrupt (location at 0003H)
	SBUF=0x1F; 
	if (phase==1){
		lickflag=1;
		if (target){correct=1;}//if mouse was supposed to lick right lead, it is incorrect, so correct =1
		else {correct=2;}}}//if mouse was supposed to lick left lead, it lick correctly, correct = 2

void exint1() interrupt 2 {	   //right lick interrupt (location at 0013H)
	SBUF=0x1E;   	
	if (phase==1){
		lickflag=1;
		if (target){correct=2;}//if mouse was supposed to lick right lead, it is incorrect, so correct=2
		else {correct=1;}}}//if mouse was supposed to lick left lead, it lick correctly, correct = 1

void Uart_Isr() interrupt 4 {
    if (RI){RI = 0; P0 = SBUF;}    	//Clear receive interrupt flag; P0 show UART data
    if (TI){TI = 0; busy = 0;}}    	//Clear transmit interrupt flag; Clear transmit busy flag 

void main(){
	unsigned char tonedifficulty[4]={0,0,0,0};
	unsigned char songdifficulty=0;
	unsigned char i=0;			//used to index tones when generating sequence
	unsigned char sequence[4];
	unsigned int j=0; 			//j is the index used to indicate which sequence is being played 		
	unsigned char code toneslist[7] = {0,1,2,3,4,5,6}; //array used to randomly generate sequences {a,b,c,d,e,f,g}
//	P1 = 0x00;
//	P3 = 0x0f;
	SCON = 0x5a;                //8 bit data, no parity bit
	TMOD = 0x21; 	            //8-bit auto reload timer 1 and 16 bit timer 0
	IE = 0x82;
//	IE = 0x9F;    				//enable global interrupt, and serial, both timer, and both external interrupts 
	TH1 = TL1 = 0xFD;           //Set Uart baudrate	(9600?)
	TL0=0x2B;                   //initial values loaded to timer 100HZ (.01s period) 
	TH0=0xB1;                   //65355-(24,000,000/12/100)=45355 45355(dec)->B12B(hex) TH0=B1 TL0=2B
	TCON= 0x50;					//start timers 0 and 1
	IT1 = 1;                    //set INT1 int type (1:Falling only 0:Low level)   not sure if these lines are necessary
   	EX1 = 1;                    //enable INT1 interrupt	
	IT0 = 1;                    //set INT0 int type (1:Falling 0:Low level)
	EX0 = 1;                    //enable INT0 interrupt
	WAKE_CLKO = 0;				//no tone generation
	srand(7);//need random seed?
	target = 0;
	while(j<=trial_number){								 		
		if (blocks & j%3 == 0){target=!target;}			//phase3 starts by deciding whether song is target or nontarget
		else if (correct == 2){target=rand()%2;}		//then generates the appropriate target/nontarget sequence
		if (target){sequence[0]=2; sequence[1]=3; sequence[2]=4; sequence[3]=5; songdifficulty=0;} 	//if target==1, sequence = cdef
		else {																		//if not, random sequence
			do{
				for (i=0;i<=3;i++){
					sequence[i] = toneslist[rand()%7];
					if (i<3){
		   				switch(sequence[i]){
							case 0: tonedifficulty[i]=abs(-3-(2*i)); break;
		    					case 1:	tonedifficulty[i]=abs(-1-(2*i)); break;
		        				case 2: tonedifficulty[i]=abs(0-(2*i)); break;
		           				case 3: tonedifficulty[i]=abs(2-(2*i)); break;
		            				case 4:	tonedifficulty[i]=4-(2*i); break;
		              				case 5:	tonedifficulty[i]=5-(2*i); break;
				              		case 6: tonedifficulty[i]=7-(2*i); break;}}
		  			else{
						switch(sequence[i]){
							case 0: tonedifficulty[i]=8; break;
			    				case 1:	tonedifficulty[i]=6; break;
				             	 	case 2: tonedifficulty[i]=5; break;
		       		    			case 3: tonedifficulty[i]=3; break;
		           		 		case 4:	tonedifficulty[i]=1; break;
		              				case 5: tonedifficulty[i]=0; break;
		             			 	case 6: tonedifficulty[i]=1; break;}}}
			songdifficulty = tonedifficulty[0]+tonedifficulty[1]+tonedifficulty[2]+tonedifficulty[3];
			}while(songdifficulty <= minimum_difficulty);}		
		while(phase==3){}//catches code until delay ends				
		j++;							//marks jth trial/song that the mouse listens
		SBUF=(unsigned char)j;																
		while(phase==0){							//song starts here
			if (tonechange){			//if timer0 adds 1 to the tone index, k, tonechange is set HIGH which indicates 
				tonechange=0;			//that it needs to switch to next tone checks which tone is supposed to play															
				switch (sequence[k]) {	//and sets BRT to the value such that it produces the appropriate frequency
					case 0: BRT = a; break; case 1: BRT = b; break; case 2: BRT = c; break;
					case 3: BRT = d; break; case 4: BRT = e; break; case 5: BRT = f; break;
					case 6: BRT = g; break;}}}						//phase 1 starts by printing out tone sequence 
		printf("%c%c",(sequence[0]<<4)|sequence[1],(sequence[2]<<4)|sequence[3]);
		while(phase==1){}		//stops code until mouse licks or runs out of time
		while(TI==0){}TI=0;
		SBUF=correct;
//		SBUF=(correct<<4)|songdifficulty;
		while(phase==2){}
		while(TI==0){}TI=0;
		SBUF=songdifficulty;	 
	}
	phase = 4;//i should disable timers here
	while(1){printf("end\n");}
}
