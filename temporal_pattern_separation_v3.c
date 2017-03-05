	//choose modes
#define  training_phase  1
	//configurable parameters
#define  punishment_duration  (unsigned int)((1)/.01)     	//set time you want the punishment to be in seconds in the parenthesises
#define  delay_duration  (unsigned int)((2)/.01) 		 
#define  tone_duration  (unsigned int)((.2)/.01)          	//set duration of tones in seconds in the parenthesises
#define  time_between_tones  (unsigned int)((.3)/.01)		//set duration of time between tones in the parenthesises
#define  lickwindow_duration  (2)/.01        				//set duration of lickwindow in seconds in the parenthesises
#define  right_valve_open_time  (.42)/.01					//set duration of open reward valve in seconds
#define  left_valve_open_time  (.43)/.01						//set duration of open reward valve in seconds
#define  trial_number  300       							//sets number of trials
#define  min_difficulty 4
#define  max_difficulty 20
#define  drip_delay_time  (.25)/.01							//only applies to trainingphase 1
//************************************************************************************************************************************

#include <reg60s2.h>
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

bit busy;					//bit used in uart interrupt
bit target=0;				//target bit used to tell if song is target=1/nontarget=0
bit tonechange=0;			//when tone is changed in timer0, tonechange goes HIGH so that main function knows to switch BRT/tone frequency
bit info_received_flag=0;
bit pause=0;
bit leftdripflag=0;
bit rightdripflag=0;
bit dripinterrupt=0;
unsigned int beginreward;
unsigned char k=0;				//index used for tones			    
unsigned char correct;			//correct=2 => mouse licked correctly;    correct=1 => incorrect;    correct=0 => no lick
unsigned int phasecounter=0;	//+1 everytime timer0 overflows, resets in timer0 when it's time for phase to change
unsigned char phase=0;    	//phase=0 song; phase=1 lickwindow; phase=2 reward; phase=3 delay
unsigned char driptimecounter=0;
unsigned char mouselicked;
sbit BRTCLKO = P2^0;			//pin 1
sbit leftlight = P2^1;			//pin 2
sbit rightlight = P2^2;			//pin 3
sbit leftvalve = P3^4;			
sbit rightvalve = P3^6; 
bit senddata=0;
//unsigned char parameters[12] = 0;
//unsigned char parameter_index = 0;
//unsigned char punishment_duration = 0;   	//set time you want the punishment to be in seconds in the parenthesises
//unsigned char delay_duration = 0; 		 
//unsigned char tone_duration = 0;         	//set duration of tones in seconds in the parenthesises
//unsigned char time_between_tones = 0;		//set duration of time between tones in the parenthesises
//unsigned char lickwindow_duration = 0;   	//set duration of lickwindow in seconds in the parenthesises
//unsigned char valve_open_time = 0;		//set duration of open reward valve in seconds
//unsigned char trial_number = 0;       	//sets number of trials
//unsigned char min_difficulty = 0;
//unsigned char max_difficulty = 0;
//unsigned char drip_delay_time = 0;		//only applies to trainingphase 1
//unsigned char training_phase = 0;	  

void timer0(void) interrupt 1 {
	TL0=0x2B;            //initial values loaded to timer 100HZ (.01s period) 
	TH0=0xB1;            //65355-(24,000,000/12/100)=45355 45355(dec)->B12B(hex) TH0=B1 TL0=2B			
	phasecounter++;
	if(training_phase==1 & dripinterrupt==0){
		if (leftdripflag==1){driptimecounter++;
			if (driptimecounter==drip_delay_time){leftlight=0; leftvalve=1;}
			else if (driptimecounter==left_valve_open_time+drip_delay_time){
				leftdripflag=0; driptimecounter=0; leftlight=1; leftvalve=0;}}
		if (rightdripflag==1){driptimecounter++; 
			if (driptimecounter==drip_delay_time){rightlight=0; rightvalve=1;}
			else if (driptimecounter==right_valve_open_time+drip_delay_time){
				rightdripflag=0; driptimecounter=0; rightlight=1; rightvalve=0;}}}
//	else{
//		if (leftdripflag==1){driptimecounter++; leftvalve=1; leftlight=0;
//			if (driptimecounter==left_valve_open_time){
//				dripinterrupt=0; leftdripflag=0; driptimecounter=0; leftlight=1; leftvalve=0;}}
//		if (rightdripflag==1){driptimecounter++; rightvalve=1; rightlight=0;
//			if (driptimecounter==right_valve_open_time){
//				dripinterrupt=0; rightdripflag=0; driptimecounter=0; rightlight=1; rightvalve=0;}}}
	if (phase==1 & phasecounter==lickwindow_duration){phasecounter=0; correct=0; phase=2; mouselicked=0;}				
	if (phase==2){			  //delay phase
		if (correct==2 & phasecounter==delay_duration){phase=0; phasecounter=0;}
		else if (phasecounter==punishment_duration+delay_duration){phase=0; phasecounter=0;}}		 //phasecounter resets every phase change}
	if (phase==0){
		if (phasecounter == 1){tonechange=1; WAKE_CLKO=0x04;}
//		else if (phasecounter == tone_duration){k++; tonechange=1; WAKE_CLKO=0;}  
//		else if (phasecounter == tone_duration+time_between_tones){WAKE_CLKO=0x04;}
//	    else if (phasecounter == 2*tone_duration+time_between_tones){k++; tonechange=1; WAKE_CLKO=0;}
//		else if (phasecounter == 2*tone_duration+2*time_between_tones){WAKE_CLKO=0x04;}
//	    else if (phasecounter == 3*tone_duration+2*time_between_tones){k++; tonechange=1; WAKE_CLKO=0;}
//	   	else if (phasecounter == 3*tone_duration+3*time_between_tones){WAKE_CLKO=0x04;}
		else if (phasecounter == 4*tone_duration+3*time_between_tones-1){
		senddata=1;}
		else if (phasecounter == 4*tone_duration+3*time_between_tones){phasecounter=0; WAKE_CLKO=0; k=0; //turn off tone; reset tone index; reset phasecounter 
			if (training_phase == 1){phase=2; 
				if (target){rightdripflag=1;}else{leftdripflag=1;}}
			else{phase=1;}}}}		//go to reward phase if training when song ends. otherwise, don't

void exint0() interrupt 0 {	   //left lick interrupt (location at 0003H)
	while(TI==0){}TI=0; SBUF=0x1F; 
	if (phase==1){phasecounter=0; phase=2; mouselicked=0x01;
		if (target){correct=1;}//if mouse was supposed to lick right lead, it is incorrect, so correct =1
		else {correct=2; leftdripflag=1;}}}//if mouse was supposed to lick left lead, it lick correctly, correct = 2

void exint1() interrupt 2 {	   //right lick interrupt (location at 0013H)
	while(TI==0){}TI=0; SBUF=0x1E;   	
	if (phase==1) {phasecounter=0; phase=2;	mouselicked=0x02;
		if (target){correct=2; rightdripflag=1;}//if mouse was supposed to lick right lead, it is incorrect, so correct=2
		else {correct=1;}}}//if mouse was supposed to lick left lead, it lick correctly, correct = 1

void Uart_Isr() interrupt 4 {
    if (RI){RI = 0; 
////	if (SBUF == 0x55) {pause~=pause}
////	else if (SBUF == 0x25) {dripinterrupt=1; rightdripflag=1; target=1;} 	//right valve drip
////	else if (SBUF == 0x52) {dripinterrupt=1; leftdripflag=1; target=0;}	//left valve drip 
////	else {parameters[0] = SBUF; parameter_index=parameter_index+1;}  
	}   
    if (TI){TI = 0; busy = 0;}}    	//Clear transmit interrupt flag; Clear transmit busy flag 

void main(){
	unsigned char targetsequence[4]={0,4,2,6};		//8th octave: {0,1,2,3,4,5,6} --> {a,b,c,d,e,f,g}
	unsigned char targetdifficulty[4]={0,0,0,0};
	unsigned char tonedifficulty=0;
	unsigned char songdifficulty=0;
	unsigned char i=0;			//used to index tones when generating sequence
	unsigned char sequence[4];
	unsigned int j1=0; 			//j1 is the index used to indicate which sequence is being played 		
	unsigned char j2=0;
	unsigned char toneslist[7]={0,1,2,3,4,5,6}; //array used to randomly generate sequences, 8th octave: {a,b,c,d,e,f,g}
	bit correctflag = 0;
	SCON=0x5a;                //8 bit data, no parity bit
	TMOD=0x21; 	            //8-bit auto reload timer 1 and 16 bit timer 0
	IE=0x82;
//	IE=0x9F;    				//enable global interrupt, and serial, both timer, and both external interrupts 
	TH1=TL1=0xFD;           //Set Uart baudrate	(9600?)
	TL0=0x2B;                   //initial values loaded to timer 100HZ (.01s period) 
	TH0=0xB1;                   //65355-(24,000,000/12/100)=45355 45355(dec)->B12B(hex) TH0=B1 TL0=2B
	TCON=0x50;					//start timers 0 and 1
	IT1=1;                    //set INT1 int type (1:Falling Edge only 0:Low level)
	IT0=1;                    //set INT0 int type (1:Falling Edge 0:Low level)		 
	AUXR=0x10; 
	P3M0=0x50;
	P3M1=0x00;
	rightvalve=0;
	leftvalve=0;
	srand(7);//need random seed?
	for (i=0;i<=3;i++){
		sequence[i] = toneslist[rand()%7];
		if (i<3){
			switch(targetsequence[i]){
				case 0: targetdifficulty[i]=0; break; case 1: targetdifficulty[i]=2; break;	case 2: targetdifficulty[i]=3; break; 	
				case 3: targetdifficulty[i]=5; break; case 4: targetdifficulty[i]=7; break; case 5: targetdifficulty[i]=8; break;
	      		case 6: targetdifficulty[i]=10; break;}}}
	while(1){
		EX1=0; EX0=0;                    //disable external interrupts
//		while(1){
//			if (parameter_index < 10){info_received_flag = 1; 
//				while(TI==0){}TI=0; SBUF = 0x55}
//			punishment_duration = parameters[0];   	//set time you want the punishment to be in seconds in the parenthesises
//			delay_duration = parameters[1]; 		 
//			tone_duration = parameters[2];         	//set duration of tones in seconds in the parenthesises
//			time_between_tones = parameters[3];		//set duration of time between tones in the parenthesises
//			lickwindow_duration = parameters[4];   	//set duration of lickwindow in seconds in the parenthesises
//		  	valve_open_time = parameters[5];		//set duration of open reward valve in seconds
//		  	trial_number = parameters[6];       	//sets number of trials
//			min_difficulty = parameters[7];
//			max_difficulty = parameters[8];
//			drip_delay_time = parameters[9];		//only applies to trainingphase 1
//			training_phase = parameters[10];
//			if (info_received_flag == 1) {info_received_flag=0; break;}
//		}
		EX1=1; EX0=1;                    //disable external interrupts	
		while(j1<=trial_number){
			if (training_phase==1){correctflag=1;}
			if (training_phase==2 & correct==2){correctflag=1;}
			if (correctflag==1 & j1%3==0){correctflag=0; target=!target;}
			else if ((training_phase==3 & correct==2) || training_phase==4){target=rand()%2;}		//song composed in phase 3
			songdifficulty=0;
			if (target==1){sequence[0]=targetsequence[0]; sequence[1]=targetsequence[1]; sequence[2]=targetsequence[2]; sequence[3]=targetsequence[3];} 	
			else {											//if target==1, sequence = cdef. if not, random sequence
				do{ for (i=0;i<3;i++){
						sequence[i]=toneslist[rand()%7];
						switch(sequence[i]){
							case 0: tonedifficulty=0; break; case 1: tonedifficulty=2; break; case 2: tonedifficulty=3; break; 	
							case 3: tonedifficulty=5; break; case 4: tonedifficulty=7; break; case 5: tonedifficulty=8; break;
				      		case 6: tonedifficulty=10; break;}
						songdifficulty=songdifficulty+abs(tonedifficulty-targetdifficulty[i]);}
				}while(songdifficulty<=min_difficulty && songdifficulty>=max_difficulty);}		
			while(phase==2){}				//catches code until delay ends				
//			while(pause==1){}
			if (j1==255 || j1==510){j2++;} j1++;  
			EX1=0; EX0=0; while(TI==0){} TI=0; SBUF=0x71;   
			while(TI==0){} TI=0; SBUF=(unsigned char)j2;
			while(TI==0){} TI=0; SBUF=(unsigned char)j1; EX1=1; EX0=1;																	
			while(phase==0){				//song starts here
				if (senddata==1){senddata=0; EX1=1; EX0=1;
					while(TI==0){} TI=0; SBUF=0x72;
					while(TI==0){} TI=0; SBUF=(sequence[0]<<4)|sequence[1];
					while(TI==0){} TI=0; SBUF=(sequence[2]<<4)|sequence[3]; EX1=0; EX0=0;}				
				if (tonechange){tonechange=0;					
					switch (sequence[k]) {	//and sets BRT to the value such that it produces the appropriate frequency
						case 0: BRT = a; break; case 1: BRT = b; break; case 2: BRT = c; break;
						case 3: BRT = d; break; case 4: BRT = e; break; case 5: BRT = f; break;
						case 6: BRT = g; break;}}}			//phase 1 starts by printing out tone sequence 
			while(phase==1){}		//stops code until mouse licks or runs out of time
			EX1=0; EX0=0; while(TI==0){} TI=0; SBUF=0x74;
			while(TI==0){} TI=0; SBUF=(correct<<4)|mouselicked;	
			while(TI==0){} TI=0; SBUF=songdifficulty; EX1=1; EX0=1;}
///////////////////////	parameter_index=0;
/////////////////////// j1=0; j2=0;
	}			
}
