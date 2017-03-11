 //choose modes
#define  training_phase  2
	//configurable parameters
#define  punishment_duration  (unsigned int)((3)/.01)     	//set time you want the punishment to be in seconds in the parenthesises
#define  delay_duration  (unsigned int)((3)/.01) 		 
#define  tone_duration  (unsigned int)((.3)/.01)          	//set duration of tones in seconds in the parenthesises
#define  time_between_tones  (unsigned int)((.2)/.01)		//set duration of time between tones in the parenthesises
#define  lickwindow_duration  (2)/.01        				//set duration of lickwindow in seconds in the parenthesises
#define  right_valve_open_time  (.3)/.01					//set duration of open reward valve in seconds
#define  left_valve_open_time  (.3)/.01						//set duration of open reward valve in seconds
#define  trial_number  300       							//sets number of trials
#define  min_difficulty 0
#define  max_difficulty 50
#define  drip_delay_time  (.00)/.01							//only applies to trainingphase 1
//************************************************************************************************************************************

#include "STC/STC12C5A60S2.H"
#include <stdlib.h> 		//for rand()/srand() function
#define FOSC 24000000L 		//quartz crystal oscillator frequency
#define BAUD 9600			//baurdrate
#define a (256-FOSC/24/7040) 	//7.04KHz frequency calculation method of 1T mode
#define b (256-FOSC/24/7902) 	//7902Hz
#define c (256-FOSC/24/8372) 	//8372Hz 
#define d (256-FOSC/24/9397) 	//9397Hz
#define e (256-FOSC/24/10540)	//10540Hz
#define f (256-FOSC/24/11175)	//11175Hz
#define g (256-FOSC/24/12544)	//12544Hz 

bit training=0;
bit busy;					//bit used in uart interrupt
bit target=0;				//target bit used to tell if song is target=1/nontarget=0
bit tonechange=0;			//when tone is changed in timer0, tonechange goes HIGH so that main function knows to switch BRT/tone frequency
bit info_received_flag=0;
bit pause=1;
bit leftdripflag=0;
bit rightdripflag=0;
bit parameterflag=1;
sbit BRTCLKO=P2^0;			//pin 1
sbit leftlight=P2^1;			//pin 2
sbit rightlight=P2^2;			//pin 3
sbit leftvalve=P3^4;			
sbit rightvalve=P3^6;
unsigned char parameters[12];
unsigned char parameter_index;
unsigned char info;
//unsigned char punishment_duration;   	//set time you want the punishment to be in seconds in the parenthesises
//unsigned char delay_duration; 		 
//unsigned char tone_duration;         	//set duration of tones in seconds in the parenthesises
//unsigned char time_between_tones;		//set duration of time between tones in the parenthesises
//unsigned char lickwindow_duration;   	//set duration of lickwindow in seconds in the parenthesises
//unsigned char valve_open_time;		//set duration of open reward valve in seconds
//unsigned char trial_number;       	//sets number of trials
//unsigned char min_difficulty;
//unsigned char max_difficulty;
//unsigned char drip_delay_time;		//only applies to trainingphase 1
//unsigned char training_phase; 
unsigned char song[4]={0,0,0,0};
unsigned char k=0;				//index used for tones			    
unsigned char correct=0;			//correct=2 => mouse licked correctly;    correct=1 => incorrect;    correct=0 => no lick
unsigned char phase=0;    	//phase=0 song; phase=1 lickwindow; phase=2 reward; phase=3 delay
unsigned char driptimecounter=0;
unsigned char mouselicked=0;
unsigned char songdifficulty=0;
unsigned int beginreward;
unsigned int phasecounter=0;	//+1 everytime timer0 overflows, resets in timer0 when it's time for phase to change

void SendData(unsigned char x,unsigned char y,unsigned char z){
	EX0=0; EX1=0;
	while(TI==0){} TI=0; SBUF=x;   
	while(TI==0){} TI=0; SBUF=y;
	while(TI==0){} TI=0; SBUF=z;
	EX0=1; EX1=1;}

char Tone2Freq(char tone_goes_here){
	unsigned char freq;
	switch(tone_goes_here){	//and sets freq to the value such that it produces the appropriate frequency
		case 0: freq = a; break; case 1: freq = b; break; case 2: freq = c; break;
		case 3: freq = d; break; case 4: freq = e; break; case 5: freq = f; break;
		case 6: freq = g; break;}
	return freq;}
					   
void timer0(void) interrupt 1 {
	TL0=0x2B;            //initial values loaded to timer 100HZ (.01s period) 
	TH0=0xB1;            //65355-(24,000,000/12/100)=45355 45355(dec)->B12B(hex) TH0=B1 TL0=2B			
	phasecounter++;
	if (leftdripflag==1){driptimecounter++; leftvalve=1; leftlight=0;
		if (driptimecounter==left_valve_open_time){
			leftdripflag=0; driptimecounter=0; leftlight=1; leftvalve=0;}}
	if (rightdripflag==1){driptimecounter++; rightvalve=1; rightlight=0;
		if (driptimecounter==right_valve_open_time){
			rightdripflag=0; driptimecounter=0; rightlight=1; rightvalve=0;}}
	if (phase==1){
		if(training_phase==1){
			if (phasecounter==drip_delay_time+1){
				if(target==1){rightlight=0; rightvalve=1;} else {leftlight=0; leftvalve=1;}}
			else if (target==1 & phasecounter==right_valve_open_time+drip_delay_time+1){rightlight=1; rightvalve=0;} 
			else if (target==0 & phasecounter==left_valve_open_time+drip_delay_time+1){leftlight=1; leftvalve=0;}}
		if(phasecounter==lickwindow_duration){phasecounter=0; phase=2; SendData(0x74,(correct<<4)|mouselicked,songdifficulty);
			if (training_phase==1 & training==1){mouselicked=0; correct=0; training=0;}
			if (training_phase!=1){mouselicked=0; correct=0;}}}
	if (phase==2){			  //delay phase
		if (correct==2 & phasecounter==delay_duration){phase=0; phasecounter=0;}
		else if (phasecounter==punishment_duration+delay_duration){phase=0; phasecounter=0;}}		 //phasecounter resets every phase change}
	if (phase==0){
		if (phasecounter==1){BRT=Tone2Freq(song[k]); WAKE_CLKO=0x04;}
		else if (phasecounter == tone_duration){k++; BRT=Tone2Freq(song[k]); WAKE_CLKO=0;}  
		else if (phasecounter == tone_duration+time_between_tones){WAKE_CLKO=0x04;}
	  else if (phasecounter == 2*tone_duration+time_between_tones){k++; BRT=Tone2Freq(song[k]); WAKE_CLKO=0;}
		else if (phasecounter == 2*tone_duration+2*time_between_tones){WAKE_CLKO=0x04;}
	 	else if (phasecounter == 3*tone_duration+2*time_between_tones){k++; BRT=Tone2Freq(song[k]); WAKE_CLKO=0;}
		else if (phasecounter == 3*tone_duration+3*time_between_tones){WAKE_CLKO=0x04;}
		else if (phasecounter==4*tone_duration+3*time_between_tones){
			SendData(0x72,(song[0]<<4)|song[1],(song[2]<<4)|song[3]); 
			phase=1; training=1; phasecounter=0; WAKE_CLKO=0; k=0;}}}

void exint0() interrupt 0 {	   //left lick interrupt (location at 0003H)
	while(TI==0){}TI=0; SBUF=0xFF; 
	if (phase==1 & training==1){mouselicked=(unsigned char)0x01; training=0;
		if (training_phase!=1){phase=2;phasecounter=0;}
		if (target){correct=(unsigned char)1;}//if mouse was supposed to lick right lead, it is incorrect, so correct =1
		else {correct=(unsigned char)2; if (training_phase!=1){leftdripflag=1;}}}
			SendData(0x74,(correct<<4)|mouselicked,songdifficulty);}//if mouse was supposed to lick left lead, it lick correctly, correct = 2

void exint1() interrupt 2 {	   //right lick interrupt (location at 0013H)
	while(TI==0){}TI=0; SBUF=0xFE;   	
	if (phase==1 & training==1) {mouselicked=(unsigned char)0x02; training=0;
		if (training_phase!=1){phase=2;phasecounter=0;}
		if (target){correct=(unsigned char)2; if (training_phase!=1){rightdripflag=1;}}//if mouse was supposed to lick right lead, it is incorrect, so correct=2
		else {correct=(unsigned char)1;}}
			SendData(0x74,(correct<<4)|mouselicked,songdifficulty);}//if mouse was supposed to lick left lead, it lick correctly, correct = 1

void Uart_Isr() interrupt 4 {
    if (RI){RI=0; info=SBUF; SBUF=info;}
//			if (info==0x55){while(1){} parameterflag=1; SendData('a','b','c');
//			parameters[0]=info; parameter_index++;}
//			else if (info==0x55){pause=!pause;}
//			else if (info==0x25) {rightdripflag=1;} 	//right valve drip
//			else if (info==0x52) {leftdripflag=1;}}	//left valve drip 		   
    if (TI){TI=0; busy=0;}}    	//Clear transmit interrupt flag; Clear transmit busy flag 

void main(){
	unsigned char tones[7]={0,1,2,3,4,5,6}; //array used to randomly generate songs,
	unsigned char targetsong[4]={0,4,2,6};		//8th octave: {0,1,2,3,4,5,6} --> {a,b,c,d,e,f,g}
	unsigned char targetdifficulty[4]={0,4,2,6};
	unsigned char tonedifficulty=0;
	unsigned char i=0;
	unsigned int j1=0; 			//j1 is the index used to indicate which song is being played 		
	bit correctflag=0;
	SCON=0x5a;         //8 bit data, no parity bit
	TMOD=0x21; 	       //8-bit auto reload timer 1 and 16 bit timer 0
	TH1=TL1=0xFD;           //Set Uart baudrate	(9600?)
	TL0=0x2B;                   //initial values loaded to timer 100HZ (.01s period) 
	TH0=0xB1;                   //65355-(24,000,000/12/100)=45355 45355(dec)->B12B(hex) TH0=B1 TL0=2B
	TCON=0x55;					//start timers 0 and 1	 
	IE=0x82;		REN=1;																						  
	AUXR=0x10; 
	P3M0=0x50;
	P3M1=0x00;
	P3=0xAF;
	EX1=0; EX0=0;//necessary?
	for (i=0;i<=3;i++){
		switch(targetsong[i]){
			case 0: targetdifficulty[i]=0; break; case 1: targetdifficulty[i]=2; break;	case 2: targetdifficulty[i]=3; break; 	
			case 3: targetdifficulty[i]=5; break; case 4: targetdifficulty[i]=7; break; case 5: targetdifficulty[i]=8; break;
      case 6: targetdifficulty[i]=10; break;}}
//		while(pause==1){
//			if (parameter_index<10){info_received_flag=1; 
//				while(TI==0){}TI=0; SBUF=0x55}
//			punishment_duration=parameters[0];   	//set time you want the punishment to be in seconds in the parenthesises
//			delay_duration=parameters[1]; 		 
//			tone_duration=parameters[2];         	//set duration of tones in seconds in the parenthesises
//			time_between_tones=parameters[3];		//set duration of time between tones in the parenthesises
//			lickwindow_duration=parameters[4];   	//set duration of lickwindow in seconds in the parenthesises
//		  	valve_open_time=parameters[5];		//set duration of open reward valve in seconds
//		  	trial_number=parameters[6];       	//sets number of trials
//			min_difficulty=parameters[7];
//			max_difficulty=parameters[8];
//			drip_delay_time=parameters[9];		//only applies to trainingphase 1
//			training_phase=parameters[10];
//			if (info_received_flag == 1) {info_received_flag=0; break;}
//		}
		EX1=1; EX0=1;                    //disable external interrupts	
		srand(TL0);
		while(1){	
			if (training_phase==1 | (training_phase==2 & correct==2)){correctflag=1;}
			if (correctflag==1 & j1%3==0){correctflag=0; target=!target;}
			if ((training_phase==3 & correct==2) || training_phase==4){target=rand()%2;}		//song composed in phase 3
			songdifficulty=0;
			if (target==1){song[0]=targetsong[0]; song[1]=targetsong[1]; song[2]=targetsong[2]; song[3]=targetsong[3];} 	
			else{do{for (i=0;i<=3;i++){											//if target==1, song = cdef. if not, random song
				song[i]=tones[rand()%7];
				switch(song[i]){
					case 0: tonedifficulty=0; break; case 1: tonedifficulty=2; break; case 2: tonedifficulty=3; break; 	
					case 3: tonedifficulty=5; break; case 4: tonedifficulty=7; break; case 5: tonedifficulty=8; break;
				  case 6: tonedifficulty=10; break;}
				songdifficulty=songdifficulty+abs(tonedifficulty-targetdifficulty[i]);}}
				while(songdifficulty<=min_difficulty && songdifficulty>=max_difficulty);}		
			while(phase==2){}				//catches code until delay ends				
			EX1=0; EX0=0;
//			while(pause==1){
//				if (parameter_index<10){info_received_flag=1; 
//					while(TI==0){}TI=0; SBUF=0x55}
//				punishment_duration=parameters[0];   	//set time you want the punishment to be in seconds in the parenthesises
//				delay_duration=parameters[1]; 		 
//				tone_duration=parameters[2];         	//set duration of tones in seconds in the parenthesises
//				time_between_tones=parameters[3];		//set duration of time between tones in the parenthesises
//				lickwindow_duration=parameters[4];   	//set duration of lickwindow in seconds in the parenthesises
//			  	valve_open_time=parameters[5];		//set duration of open reward valve in seconds
//			  	trial_number=parameters[6];       	//sets number of trials
//				min_difficulty=parameters[7];
//				max_difficulty=parameters[8];
//				drip_delay_time=parameters[9];		//only applies to trainingphase 1
//				training_phase=parameters[10];
//				if (info_received_flag==1) {info_received_flag=0; break;}} 
			EX1=1; EX0=1;
			j1++; SendData(0x71,(j1>>8)&0xff,j1);
			if(j1==trial_number){pause=1; parameter_index=0; j1=0;}																	
			while(phase==0){}			
			while(phase==1){}		
			}
	}
