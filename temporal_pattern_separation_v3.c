//implement double lick and minumum difficulty

//include headers
#include <reg60s2.h>
#include <stdio.h>
#include <intrins.h>
#include <math.h>
#include <stdlib.h>


#define FOSC    24000000L 
#define BAUD    9600

	//************************************************************************************************************************************
	//choose modes
#define  delay_onoff  0      //set to 1 to delay mouse if it doesn't lick correctly or at all; set 0 otherwise
#define  blocks  0           //set to 1 if you want to use the block training protocol; set to 0 otherwise
#define  training  0		       	//set to 1 if you want the appropriate reward valve to open after every sequence of tones
	//configurable parameters
#define  duration_delay  (5)/.02                 //set time you want the delay to be in seconds in the parenthesises
#define  duration_tones  (.2)/.02                 //set duration of tones in seconds in the parenthesises
#define  duration_time_between_tones  (.3)/.02     //set duration of time between tones in the parenthesises
#define  duration_lickwindow  (2)/.02              //set duration of lickwindow in seconds in the parenthesises
#define  duration_open_valve  (.1)/.02	            //set duration of open reward valve in seconds in the parenthesises
#define  trial_number  200                      //sets number of trials
	//************************************************************************************************************************************
//pins are configured here
sbit TONE = P1^0; 								//output pins
sbit leftvalve = P1^2;
sbit rightvalve = P1^1;
bit busy;
P1_1 = 1;								         	//input pins
P1_2 = 1;	


//global variables 
unsigned char TH0BIT; 					//TxOBITs used to set frequency of square wave. Calculation for 10,000HZ wave  
unsigned char TL0BIT;			  		//(.0001s period) 65535-(24000000/12/10000)=65335(dec)->FF37(hex) TH0=FF TL0=37
unsigned int continuouscounter = 0;			/*counts number of times that timer 1 resets without resetting to know the overall time 
	                     				lickwindow, etc. and resets to zero each time the program switches between these phases
					                  	passes in the duration of the program*/
unsigned int timer0flag = 0;               //set to 1 everytime timer1 overflows

//void timer2(void) interrupt 1	{ 	      //timer 0 interrupt used for generating square wave for tones
//  TH0=TH0BIT;                         // timer reloaded 
// 	TL0=TL0BIT;   
//	TONE=~TONE;}                        //toggle or untoggle TONE on interrupt

void timer0(void) interrupt 1 {   /* timer 1 interrupt used to count how much time passes so that it knows when to: 
                                     play or stop playing tones, start lickwindow, stop lickwindow, start delay, etc. */
	TL0=0x0B;                       // initial values loaded to timer 50HZ (.02s period) 
	TH0=0x63;                       // 65355-(24,000,000/12/50)=25355 25355(dec)->630B(hex) TH1=63 TL1=0B
	timer0flag = 1;
	continuouscounter++;
}	

void Uart_Isr() interrupt 4 using 1
{
    if (RI)
    {
        RI = 0;             //Clear receive interrupt flag
        P0 = SBUF;          //P0 show UART data
    }
    if (TI)
    {
        TI = 0;             //Clear transmit interrupt flag
        busy = 0;           //Clear transmit busy flag
    }
}


void dripfunction(int  targets, int corrects, int trainings, int durationvalves, int timecounter){
	int driptimer0counter = 0;
	int time;
	int rightlick;
	int leftlick;
	if (corrects == 1 || trainings == 1){
		if (targets == 0){
			rightvalve = 1;
		}
		else{
			leftvalve = 1;
		}
	}
	while (corrects == 1 || trainings == 1){
		rightlick = P1_1;
		leftlick = P1_2;
		time = (.02*timecounter) + (.0000005*(65355-(TH0<<2|TL0)+25335));
		printf("\n%u,%u,%u",time,leftlick,rightlick);
		if (timer0flag == 1){
			driptimer0counter++;
			timer0flag = 0;
			if (driptimer0counter == durationvalves){
				driptimer0counter = 0;
				rightvalve = 0;
				leftvalve = 0;
			}
		}
	}
}

void main(){ 
	unsigned int timerzz;
		//other variables    
	char xdata sequence[200][4];
	const int duration_sequence = 4*duration_tones + 3*duration_time_between_tones;
	const int duration_tones_and_space = duration_tones + duration_time_between_tones;
	unsigned int  timer0counter;
	unsigned int  j; 					          	//j is the index used to indicate which sequence is being played
	unsigned char  phase = 0;    //phase = 0 means indicates that the tones are playing
                    //phase = 1 indicates that the lickwindow is on
                    //phase = 2 indictates that it is the delay time between lickwindow and next song
  unsigned int  k;				        	    //k is the index used to indicate which tone is playing of the trial_numberth sequence
  unsigned char  whatmouselicks;
	unsigned char  sequencelick;						//sequencelick=5 indicates that the mouse licked left; 6 --> right; 4 --> no lick; 7 --> licked both
	unsigned char  realtimelick;						//
	unsigned char  leftlick = 0;			      //if leftlick = 1, mouse made contact with left lick port
	unsigned char  rightlick = 0;			    //if leftlick = 1, mouse made contact with right lick port
	unsigned char  leftlickcounter = 0;		//counts the number of consecutive times in which the left lick ports reads HIGH
	unsigned char  rightlickcounter = 0;		//counts the number of consecutive times in which the right lick ports reads HIGH
	unsigned char  whatmouseshouldlick;        
	unsigned char  correct = 0;				//correct=1 indicates mouse licked correctly; correct = 0 incorrect lick; correct = -1 indicates no lick
  unsigned char  target = 0;				      //if target = 0, target sequence plays. if target = 1, nontarget sequence plays
    unsigned int  time = 0;
  unsigned char  xdata block_sequence[300];
  unsigned char code toneslist[8] = {'a','b','c','d','e','f','g','\0'};		//array used to randomly generate sequences
	SCON = 0x5a;                    //8 bit data ,no parity bit
	TMOD = 0x20;                    //T1 as 8-bit auto reload
	TH1 = TL1 = 0xFD; //Set Uart baudrate
	TR1 = 1;                        //T1 start running																										//enable serial receive --- 8-bit Shift Register Mode: (baud rate = crystal/12)
	IE = 0x82;    														//enable global interrupt and timer0 and timer1 and serial interrupts 
	TMOD = 0x21; 	                            //16 bit timers for timer 1 and timer 0
	TL0=0x0B;                                 //initial values loaded to timer 50HZ (.02s period) 
	TH0=0x63;                                 //65355-(24,000,000/12/50)=25355 25355(dec)->630B(hex) TH1=63 TL1=0B
	TR0 = 1;	
	for (j = 0 ; j <= trial_number ; j++){						
		for (k = 0 ; k <= 3 ; k++){		//this loop generates nontarget sequence in the second column of sequence
			unsigned int randindex = rand() % 7;
			sequence[j][k] = toneslist[randindex];
	  }
	printf("song=%c%c%c%c \n",sequence[j][0],sequence[j][1],sequence[j][2],sequence[j][3]);
	}
	for (j = 0 ; j <= trial_number ; j++){							
		if (j%3 == 2){
			k = !k;
	  }
		block_sequence[j] = k;
	}
	//need random seed?
	srand(7);																	//seed rng
	for(j = 0 ; j < trial_number ; j++){
		if (blocks == 1){
			target = block_sequence[j];
		}
		if (target == 0){
			sequence[j][0] = 'c';        //if it's supposed to play target sequence
			sequence[j][1] = 'd';        //it overwrites the nontarget sequence that was
			sequence[j][2] = 'e';		    //that was there before
			sequence[j][3] = 'f';
		}
		k = 0;
		while(phase == 0){
//			printf("song\n");
			leftlick = P1_2;											    	//read the input lick pins
			rightlick = P1_1;
			if (leftlick == 0 & rightlick == 0){
				realtimelick = 3;
			}
			else if (leftlick == 0 & rightlick == 1){
				realtimelick = 5;
			}
			else if (leftlick == 1 & rightlick == 0){
				realtimelick = 4;
			}
			else {
				realtimelick = 6;
			}  





			//I'm having trouble printing the time right here
			//comment out this loop if you want to look at the next part I'm having trouble with 
			//Also, I don't know if this is the best way to do it, having three placeholders
			while(1){
			time = (.02*continuouscounter);
			timerzz=(.0000005*(45355-(TH0<<4|TL0)));
			printf("time = %d.%.2d%.7d \n", time,2*continuouscounter%50,timerzz);
			printf("continuouscounter = %u \n", continuouscounter);}




			 //this is where i'm having trouble with my timer0counter. when I try to print it this way,
			 //I only get "timer0counter=1".
			 //my continuouscounter increases as it should.
   			timer0counter = 0;
  			while(1){
			if (timer0flag == 1){                       //if timer0flag goes HIGH, increase timer0counter and check if the tone should
				timer0counter++;
				printf("timer0counter=%u\n",timer0counter);						        //start/stop playing, change, or move onto lickwindow
				timer0flag = 0;}







//			    if (timer0counter == (k+1)*duration_tones){
//			       TR0 = 0; 	                              //toggle timer 0 which toggles tone
//				}
//			    if (timer0counter == (k+1)*duration_tones_and_space){
//			     	k++;
//			    	printf("k=%u", k);
//					if (timer0counter == duration_sequence){
//				       phase = 1;
//					   timer0counter = 0;
//					   TR0 = 1;
//				    }
//					else{
//						switch (sequence[j][k]) {
//							case 'a' : TH0BIT = 0xFE; TL0BIT = 0xE3; break;
//							case 'b' : TH0BIT = 0xFF; TL0BIT = 0x02; break;
//							case 'c' : TH0BIT = 0xFF; TL0BIT = 0x10; break;
//							case 'd' : TH0BIT = 0xFF; TL0BIT = 0x2A; break;
//							case 'e' : TH0BIT = 0xFF; TL0BIT = 0x41; break;
//							case 'f' : TH0BIT = 0xFF; TL0BIT = 0x4C; break;
//							case 'g' : TH0BIT = 0xFF; TL0BIT = 0x60; break;
//						}
//					}					
//		    }
		  }
		}	
//		while (training == 1 && phase == 1){
//		printf("training lickwindow");
//			dripfunction(target, correct, training, duration_open_valve, continuouscounter);
//			leftlick = P1_2;
//			rightlick = P1_1;
//			if (leftlick == 0 & rightlick == 0){
//				realtimelick = 3;
//			}
//			else if (leftlick == 0 & rightlick == 1){
//				realtimelick = 5;
//			}
//			else if (leftlick == 1 & rightlick == 0){
//				realtimelick = 4;
//			}
//			else {
//				realtimelick = 6;
//			}  
//			time = (.02*continuouscounter) + (.0000005*(65355-(TH0<<2|TL0)+25335));
//      printf("\n%u,%u,%u",time,phase,realtimelick);
//			if (timer0flag == 1){
//				timer0counter++;
//				timer0flag = 0;
//				if (timer0counter == duration_lickwindow){
//					timer0counter = 0;
//					phase = 2;
//				}
//			}
//		}
//		while(training == 0 && phase == 1){
//		printf("regular lickwindow");
//      leftlick = P1_2;
//			rightlick = P1_1;
//			if (leftlick == 0 & rightlick == 0){
//				realtimelick = 3;
//			}
//			else if (leftlick == 0 & rightlick == 1){
//				realtimelick = 5;
//			}
//			else if (leftlick == 1 & rightlick == 0){
//				realtimelick = 4;
//			}
//			else {
//				realtimelick = 6;
//			}  
//			time = (.02*continuouscounter) + (.0000005*(65355-(TH0<<2|TL0)+25335));
//      printf("\n%u,%u,%u",time,phase,realtimelick);
//			if (leftlick == 1){
//				leftlickcounter = leftlickcounter++;			  
//				if (leftlickcounter >= 10){
//					leftlickcounter = 0;
//					sequencelick = 1;
//					timer0counter = 0;
//					if (target == 1){
//						correct = 1;
//						phase = 0;
//						whatmouselicks = 16;
//						dripfunction(target, correct, training, duration_open_valve, continuouscounter);
//  				  whatmouseshouldlick = target;
//				  target = rand()%2;
//					}
//					else{
//						correct = 0;
//						phase = 2;
//					}
//					timer0counter = 0;
//				}
//			}
//  		else{
//		   leftlickcounter = 0;
//	  	}
//			if (rightlick == 1){
//				rightlickcounter = rightlickcounter++;
//				if (rightlickcounter >= 10){
//					rightlickcounter = 0;
//					sequencelick = 2;
//					whatmouselicks = 18;
//					if (target == 0){
//						correct = 1;
//						phase = 0;
//						dripfunction(target, correct, training, duration_open_valve, continuouscounter);
//  				  whatmouseshouldlick = target;
//				  target = rand()%2;
//					}
//					else{
//						correct = 0;
//						phase = 2;
//					}
//		      timer0counter = 0;
//				}
//			}
//			else{
//				rightlickcounter = 0;
//			}
//			if (timer0flag == 1){
//				timer0counter++;
//				timer0flag = 0;
//				if (timer0counter == duration_lickwindow){
//					whatmouselicks = 14;
//					timer0counter = 0;
//					correct = 0;
//					phase = 2;
//				}
//			}
//		}
//		if (delay_onoff == 1){
//			while (phase == 2){
//			printf("delay");
//                leftlick = P1_2;
//				rightlick = P1_1;
//				if (timer0flag == 1){
//					timer0counter++;
//					timer0flag = 0;
//					if (timer0counter == duration_delay){
//						phase = 0;
//						timer0counter = 0;
//					}
//				}
//				time = (.02*continuouscounter) + (.0000005*(65355-(TH0<<2|TL0)+25335));
//	      printf("\n%u,%u,%u",time,phase,realtimelick);
//			}
//		}
//		else{
//			phase = 0;
//			timer0counter = 0;
//		}
//		printf("\n,,,%u,%u,%u,%s",j,whatmouselicks,whatmouseshouldlick,sequence[trial_number]);
	}
//	while(1){
//	printf("end");
//	}
}
				driptimer1counter = 0;
				rightvalve = 0;
				leftvalve = 0;
			}
		}
	}
}

