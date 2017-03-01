	//choose modes
#define  training_phase  1
	//configurable parameters
#define  punishment_duration  (unsigned int)((1)/.01)     	//set time you want the punishment to be in seconds in the parenthesises
#define  delay_duration  (unsigned int)((.5)/.01) 		 
#define  tone_duration  (unsigned int)((.2)/.01)          	//set duration of tones in seconds in the parenthesises
#define  time_between_tones  (unsigned int)((.3)/.01)		//set duration of time between tones in the parenthesises
#define  lickwindow_duration  (2)/.01        				//set duration of lickwindow in seconds in the parenthesises
#define  valve_open_time  (.4)/.01							//set duration of open reward valve in seconds
#define  right_valve_open_time  (.4)/.01							//set duration of open reward valve in seconds
#define  left_valve_open_time  (.4)/.01							//set duration of open reward valve in seconds
#define  trial_number  200       							//sets number of trials
#define  min_difficulty 4
#define  max_difficulty 20
#define  drip_delay_time  (.5)/.01							//only applies to trainingphase 1
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
bit target;					//target bit used to tell if song is target=1/nontarget=0
bit tonechange=0;			//when tone is changed in timer0, tonechange goes HIGH so that main function knows to switch BRT/tone frequency
bit lickflag=0;
bit info_received_flag=0;
unsigned int beginreward;
unsigned char k=0;				//index used for tones			    
unsigned char correct;			//correct=2 => mouse licked correctly;    correct=1 => incorrect;    correct=0 => no lick
unsigned int phasecounter = 0;	//+1 everytime timer0 overflows, resets in timer0 when it's time for phase to change
unsigned char phase = 0;    	//phase=0 song; phase=1 lickwindow; phase=2 reward; phase=3 delay
sbit BRTCLKO = P2^0;			//pin 1
sbit leftlight = P2^1;			//pin 2
sbit rightlight = P2^2;			//pin 3
sbit leftvalve = P3^4;			
sbit rightvalve = P3^6; 
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
	if(phase==1){
		if (phasecounter == lickwindow_duration){			//if no lick after 2 sec in not training mode,
			phasecounter=0; correct=0; phase=3;}			//go to delay phase, set correct=0, phasecounter=0
		else if (lickflag){lickflag=0; phasecounter=0;		//if mouse licks, phasecounter=0
			if (correct==2){phase=2;}						//go to reward phase if correct				
			else if (correct==1){phase=3;}}}				//if not, go to delay phase  
	if (phase == 2){
		if (training_phase == 1){				
			if (phasecounter == drip_delay_time){  		
				if (target){rightlight=0; rightvalve=1;} else {leftlight=0; leftvalve=1;}}				//reward mouse from left or right lead  
			else if (phasecounter == drip_delay_time+valve_open_time){	
				rightlight=1; rightvalve=0; leftlight=1; leftvalve=0; phase=3; phasecounter=0;}}		//change phase, reset phasecounter, reset dripflag
		else { 		
			if (target){rightlight=0;} else {leftlight=0;}				//reward mouse from left or right lead  
			if (phasecounter == valve_open_time){	
				rightlight=1; leftlight=1; phase=3; phasecounter=0;}}}		//change phase, reset phasecounter, reset dripflag
	if (phase == 3){			  //delay phase
		if (correct==2 & phasecounter==delay_duration){phase=0; phasecounter=0;}
		else if (phasecounter==punishment_duration+delay_duration){phase=0; phasecounter=0;}}		 //phasecounter resets every phase change}
	if (phase==0){
		if (phasecounter == 1){tonechange=1; WAKE_CLKO=0x04;}
		else if (phasecounter == tone_duration){k++; tonechange=1; WAKE_CLKO=0;}  
		else if (phasecounter == tone_duration+time_between_tones){WAKE_CLKO=0x04;}
	    else if (phasecounter == 2*tone_duration+time_between_tones){k++; tonechange=1; WAKE_CLKO=0;}
		else if (phasecounter == 2*tone_duration+2*time_between_tones){WAKE_CLKO=0x04;}
	    else if (phasecounter == 3*tone_duration+2*time_between_tones){k++; tonechange=1; WAKE_CLKO=0;}
	   	else if (phasecounter == 3*tone_duration+3*time_between_tones){WAKE_CLKO=0x04;}
		else if (phasecounter == 4*tone_duration+3*time_between_tones){ 							//song ends
			phasecounter=0; WAKE_CLKO=0; k=0;							//turn off tone; reset tone index; reset phasecounter 
			if (training_phase == 1){phase=2;}else{phase=1;}}}}		//go to reward phase if training when song ends. otherwise, don't

void exint0() interrupt 0 {	   //left lick interrupt (location at 0003H)
	while(TI==0){}TI=0; SBUF=0x1F; 
	if (phase==1){
		lickflag=1;
		if (target){correct=1;}//if mouse was supposed to lick right lead, it is incorrect, so correct =1
		else {correct=2;}}}//if mouse was supposed to lick left lead, it lick correctly, correct = 2

void exint1() interrupt 2 {	   //right lick interrupt (location at 0013H)
	while(TI==0){}TI=0; SBUF=0x1E;   	
	if (phase==1){
		lickflag=1;
		if (target){correct=2;}//if mouse was supposed to lick right lead, it is incorrect, so correct=2
		else {correct=1;}}}//if mouse was supposed to lick left lead, it lick correctly, correct = 1

void Uart_Isr() interrupt 4 {
    if (RI){RI = 0; 
////	if (SBUF == 0x55) {pauseplay==1} else {parameters[0] = SBUF; parameter_index=parameter_index+1;}  
	}    	//Clear receive interrupt flag; P0 show UART data
    if (TI){TI = 0; busy = 0;}}    	//Clear transmit interrupt flag; Clear transmit busy flag 

void main(){
	unsigned char tonedifficulty[4]={0,0,0,0};
	unsigned char songdifficulty=0;
	unsigned char i=0;			//used to index tones when generating sequence
	unsigned char sequence[4];
	unsigned int j=0; 			//j is the index used to indicate which sequence is being played 		
	unsigned char code toneslist[7] = {0,1,2,3,4,5,6}; //array used to randomly generate sequences {a,b,c,d,e,f,g}
	bit correctflag = 0;
	SCON = 0x5a;                //8 bit data, no parity bit
	TMOD = 0x21; 	            //8-bit auto reload timer 1 and 16 bit timer 0
	IE = 0x82;
//	IE = 0x9F;    				//enable global interrupt, and serial, both timer, and both external interrupts 
	TH1 = TL1 = 0xFD;           //Set Uart baudrate	(9600?)
	TL0=0x2B;                   //initial values loaded to timer 100HZ (.01s period) 
	TH0=0xB1;                   //65355-(24,000,000/12/100)=45355 45355(dec)->B12B(hex) TH0=B1 TL0=2B
	TCON= 0x50;					//start timers 0 and 1
	IT1 = 1;                    //set INT1 int type (1:Falling Edge only 0:Low level)
	IT0 = 1;                    //set INT0 int type (1:Falling Edge 0:Low level)		 
	AUXR = 0x10;                //BRT work in 1T mode
	P3M0=0x50;
	P3M1=0x00;
	srand(7);//need random seed?
	target = 0;
//	P3=0;
	rightvalve=0;
	leftvalve=0;
	while(1){
		EX1 = 0;                    //enable INT1 interrupt
    	EX0 = 0;                    //enable INT0 interrupt	
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
//			if (info_received_flag == 1) {info_received_flag = 0; break;}
//		}
		EX1 = 1;                    //enable INT1 interrupt
    	EX0 = 1;                    //enable INT0 interrupt	
		while(j<=trial_number){
			if (correct == 2) {correctflag=1;}								 		
			if (training_phase == 1 & j%3 == 0) {target=!target;}
			else if (training_phase == 2 & correctflag == 1  & j%3 == 0){correctflag=0; target=!target;}
			else if ((training_phase == 3 & correct == 2) || training_phase == 4){target=rand()%2;}		//song composed in phase 3
			if (target){sequence[0]=2; sequence[1]=3; sequence[2]=4; sequence[3]=5; songdifficulty=0;} 	
			else {											//if target==1, sequence = cdef. if not, random sequence
				do{ for (i=0;i<=3;i++){
						sequence[i] = toneslist[rand()%7];
						if (i<3){
			   				switch(sequence[i]){
								case 0: tonedifficulty[i]=abs(-3-(2*i)); break; case 1:	tonedifficulty[i]=abs(-1-(2*i)); break;
			        			case 2: tonedifficulty[i]=abs(0-(2*i)); break; 	case 3: tonedifficulty[i]=abs(2-(2*i)); break;
			            		case 4:	tonedifficulty[i]=4-(2*i); break; 		case 5: tonedifficulty[i]=5-(2*i); break;
			              		case 6: tonedifficulty[i]=7-(2*i); break;}}
			  			else{
							switch(sequence[i]){
								case 0: tonedifficulty[i]=8; break; case 1:	tonedifficulty[i]=6; break;
			             	 	case 2: tonedifficulty[i]=5; break;	case 3: tonedifficulty[i]=3; break;
			            		case 4:	tonedifficulty[i]=1; break;	case 5: tonedifficulty[i]=0; break;
			             	 	case 6: tonedifficulty[i]=1; break;}}}
				songdifficulty = tonedifficulty[0]+tonedifficulty[1]+tonedifficulty[2]+tonedifficulty[3];
				}while(songdifficulty <= min_difficulty && songdifficulty >= max_difficulty);}		
			while(phase==3){}				//catches code until delay ends				
			j++; SBUF=(unsigned char)j;		//marks jth song that the mouse listens																
			while(phase==0){				//song starts here
				if (tonechange){			//if timer0 adds 1 to the tone index, k, tonechange is set HIGH which indicates 
					tonechange=0;			//that it needs to switch to next tone checks which tone is supposed to play						
					switch (sequence[k]) {	//and sets BRT to the value such that it produces the appropriate frequency
						case 0: BRT = a; break; case 1: BRT = b; break; case 2: BRT = c; break;
						case 3: BRT = d; break; case 4: BRT = e; break; case 5: BRT = f; break;
						case 6: BRT = g; break;
						}}}			//phase 1 starts by printing out tone sequence 
			while(TI==0){}TI=0; SBUF=(sequence[0]<<4)|sequence[1];
			while(TI==0){}TI=0; SBUF=(sequence[2]<<4)|sequence[3];
			while(phase==1){}		//stops code until mouse licks or runs out of time
			while(TI==0){}TI=0; SBUF=correct;
			while(phase==2){}		//reward
			while(TI==0){}TI=0; SBUF=songdifficulty;	 
		}
///////////////////////	parameter_index=0;
	}			
}
