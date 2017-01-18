//implement minumum difficulty
//both lick for sequence data

//include headers
#include <reg51.h>

//pins are configured here
sbit TONE = P1^0; 								//output pins
sbit leftvalve = P1^2;
sbit rightvalve = P1^1;
P1_1 = 1;								         	//input pins
P1_2 = 1;	


//global variables 
int TH0BIT; 					//TxOBITs used to set frequency of square wave. Calculation for 10,000HZ wave  
int TL0BIT;			  		//(.0001s period) 65535-(24000000/12/10000)=65335(dec)->FF37(hex) TH0=FF TL0=37
int continuouscounter = 0;			/*counts number of times that timer 1 resets without resetting to know the overall time 
	                     				lickwindow, etc. and resets to zero each time the program switches between these phases
					                  	passes in the duration of the program*/
int timer1flag;               //set to 1 everytime timer1 overflows

void timer0(void) interrupt 1	{	      //timer 0 interrupt used for generating square wave for tones
  TH0=TH0BIT;                         // timer reloaded 
 	TL0=TL0BIT;   
	TONE=~TONE;}                        //toggle or untoggle TONE on interrupt

void timer1(void) interrupt 3 {   /* timer 1 interrupt used to count how much time passes so that it knows when to: 
                                     play or stop playing tones, start lickwindow, stop lickwindow, start delay, etc. */
	TL1=0x0B;                       // initial values loaded to timer 50HZ (.02s period) 
	TH1=0x63;                       // 65355-(24,000,000/12/50)=25355 25355(dec)->630B(hex) TH1=63 TL1=0B
	timer1flag = 1;
	continuouscounter++;
}	

int printf(const char *format, ... );
int rand(void);  									//random number generator
void srand(unsigned int seed);    //sets a seed to rng

void dripfunction(int  targets, int corrects, int trainings, int durationvalves, int timecounter){
	int driptimer1counter = 0;
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
		printf("\n%n,%n,%n",time,leftlick,rightlick);
		if (timer1flag == 1){
			driptimer1counter++;
			timer1flag = 0;
			if (driptimer1counter == durationvalves){
				driptimer1counter = 0;
				rightvalve = 0;
				leftvalve = 0;
			}
		}
	}
}

main(){ 
	
	
	//************************************************************************************************************************************
	//choose modes
	const int delay_onoff = 0;	      //set to 1 to delay mouse if it doesn't lick correctly or at all; set 0 otherwise
  const int blocks = 0;             //set to 1 if you want to use the block training protocol; set to 0 otherwise
  const int training = 0;		       	//set to 1 if you want the appropriate reward valve to open after every sequence of tones
	//configurable parameters
	const int duration_delay = (5)/.02;                   //set time you want the delay to be in seconds in the parenthesises
	const int duration_tones = (.2)/.02;                  //set duration of tones in seconds in the parenthesises
  const int duration_time_between_tones = (.3)/.02;     //set duration of time between tones in the parenthesises
	const int duration_lickwindow = (2)/.02;              //set duration of lickwindow in seconds in the parenthesises
	const int duration_open_valve = (.1)/.02;	            //set duration of open reward valve in seconds in the parenthesises
	const int trial_number = 200;		                      //sets number of trials
	//************************************************************************************************************************************
	
	
	//other variables    
	char xdata sequence[200][2][4];	            /*sequence[trial_number][x][k] x=0 contains a bunch of  nontarget  
																	  	        sequences; x=1 contains the sequence that is actually played;*/
	const int duration_sequence = 4*duration_tones + 3*duration_time_between_tones;
	const int duration_tones_and_space = duration_tones + duration_time_between_tones;
	int timer1counter;
	int j; 					          	//j is the index used to indicate which sequence is being played
	int phase = 0;    //phase = 0 means indicates that the tones are playing
                    //phase = 1 indicates that the lickwindow is on
                    //phase = 2 indictates that it is the delay time between lickwindow and next song
  int k;				        	    //k is the index used to indicate which tone is playing of the trial_numberth sequence
    	int whatmouselicks;
	int sequencelick;						//sequencelick=5 indicates that the mouse licked left; 6 --> right; 4 --> no lick; 7 --> licked both
	int xdata realtimelick;						//
	int leftlick = 0;			      //if leftlick = 1, mouse made contact with left lick port
	int xdata rightlick = 0;			    //if leftlick = 1, mouse made contact with right lick port
	int xdata leftlickcounter = 0;		//counts the number of consecutive times in which the left lick ports reads HIGH
	int xdata rightlickcounter = 0;		//counts the number of consecutive times in which the right lick ports reads HIGH
	int xdata whatmouseshouldlick;        
	int correct = 0;				//correct=1 indicates mouse licked correctly; correct = 0 incorrect lick; correct = -1 indicates no lick
  int target = 0;				      //if target = 0, target sequence plays. if target = 1, nontarget sequence plays
	int time = 0;
  int xdata block_sequence[300];
  const char xdata toneslist[8] = {'a','b','c','d','e','f','g','\0'};		//array used to randomly generate sequences
	for (j = 0 ; j <= trial_number ; j++){							
		for (k = 0 ; k <= 3 ; k++){		//this loop generates nontarget sequence in the second column of sequence
			int randindex = rand() % 6;
			sequence[trial_number][0][k] = toneslist[randindex];
	  }
	}
	for (j = 0 ; j <= trial_number ; j++){							
		if (j%3 == 2){
			k = !k;
	  }
		block_sequence[j] = k;
	}
	//need random seed?
	srand(7);																	//seed rng
	SCON=0x10;                                //enable serial receive --- 8-bit Shift Register Mode: (baud rate = crystal/12)
	IE = 0x8A;    														//enable global interrupt and timer0 and timer1 and serial interrupts 
	TMOD = 0x11; 	                            //16 bit timers for timer 1 and timer 0
	TL1=0x0B;                                 //initial values loaded to timer 50HZ (.02s period) 
	TH1=0x63;                                 //65355-(24,000,000/12/50)=25355 25355(dec)->630B(hex) TH1=63 TL1=0B
	TR1 = 1;						                      //start timer 1
	for(j = 0 ; j < trial_number ; j++){
		if (blocks == 1){
			target = block_sequence[j];
		}
		if (target == 0){
			sequence[j][0][0] = 'c';        //if it's supposed to play target sequence
			sequence[j][0][1] = 'd';        //it overwrites the nontarget sequence that was
			sequence[j][0][2] = 'e';		    //that was there before
			sequence[j][0][3] = 'f';
		}
		k = 0;
		while(phase == 0){
			leftlick = P1_2;											    	//read the input lick pins
			rightlick = P1_1;
			time = (.02*continuouscounter) + (.0000005*(65355-(TH0<<2|TL0)+25335));
      printf("\n%n,%n,%n",time,phase,realtimelick);
			if (timer1counter == 0){
				TR0 = 1;
			}
			if (timer1flag == 1){                       //if timer1flag goes HIGH, increase timer1counter and check if the tone should
				timer1counter++;		  						        //start/stop playing, change, or move onto lickwindow
				timer1flag = 0;
			  if (timer1counter == (k+1)*duration_tones){
			    TR0 = 0; 	                              //toggle timer 0 which toggles tone
				}
				else if (timer1counter == (k+1)*duration_tones_and_space){
			  	k++;
					if (timer1counter == duration_sequence){
					  phase = 1;
					  timer1counter = 0;
					  TR0 = 1;
				  }
					else{
						switch (sequence[j][1][k]) {
							case 'a' : TH0BIT = 0xFE; TL0BIT = 0xE3; break;
							case 'b' : TH0BIT = 0xFF;	TL0BIT = 0x02; break;
							case 'c' : TH0BIT = 0xFF;	TL0BIT = 0x10; break;
							case 'd' : TH0BIT = 0xFF; TL0BIT = 0x2A; break;
							case 'e' : TH0BIT = 0xFF; TL0BIT = 0x41; break;
							case 'f' : TH0BIT = 0xFF; TL0BIT = 0x4C; break;
							case 'g' : TH0BIT = 0xFF; TL0BIT = 0x60; break;
						}
					}					
		    }
		  }
		}	
		while (training == 1 && phase == 1){
			dripfunction(target, correct, training, duration_open_valve, continuouscounter);
			leftlick = P1_2;
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
			time = (.02*continuouscounter) + (.0000005*(65355-(TH0<<2|TL0)+25335));
      printf("\n%n,%n,%n",time,phase,realtimelick);
			if (timer1flag == 1){
				timer1counter++;
				timer1flag = 0;
				if (timer1counter == duration_lickwindow){
					timer1counter = 0;
					phase = 2;
				}
			}
		}
		while(training == 0 && phase == 1){
      leftlick = P1_2;
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
			time = (.02*continuouscounter) + (.0000005*(65355-(TH0<<2|TL0)+25335));
      printf("\n%n,%n,%n",time,phase,realtimelick);
			if (leftlick == 1){
				leftlickcounter = leftlickcounter++;			  
				if (leftlickcounter >= 10){
					leftlickcounter = 0;
					sequencelick = 1;
					timer1counter = 0;
					if (target == 1){
						correct = 1;
						phase = 0;
						whatmouselicks = 16;
						dripfunction(target, correct, training, duration_open_valve, continuouscounter);
  				  whatmouseshouldlick = target;
				  target = rand()%2;
					}
					else{
						correct = 0;
						phase = 2;
					}
					timer1counter = 0;
				}
			}
  		else{
		   leftlickcounter = 0;
	  	}
			if (rightlick == 1){
				rightlickcounter = rightlickcounter++;
				if (rightlickcounter >= 10){
					rightlickcounter = 0;
					sequencelick = 2;
					whatmouselicks = 18;
					if (target == 0){
						correct = 1;
						phase = 0;
						dripfunction(target, correct, training, duration_open_valve, continuouscounter);
  				  whatmouseshouldlick = target;
				  target = rand()%2;
					}
					else{
						correct = 0;
						phase = 2;
					}
		      timer1counter = 0;
				}
			}
			else{
				rightlickcounter = 0;
			}
			if (timer1flag == 1){
				timer1counter++;
				timer1flag = 0;
				if (timer1counter == duration_lickwindow){
					whatmouselicks = 14;
					timer1counter = 0;
					correct = 0;
					phase = 2;
				}
			}
		}
		if (delay_onoff == 1){
			while (phase == 2){
        leftlick = P1_2;
				rightlick = P1_1;
				if (timer1flag == 1){
					timer1counter++;
					timer1flag = 0;
					if (timer1counter == duration_delay){
						phase = 0;
						timer1counter = 0;
					}
				}
				time = (.02*continuouscounter) + (.0000005*(65355-(TH0<<2|TL0)+25335));
	      printf("\n%n,%n,%n",time,phase,realtimelick);
			}
		}
		else{
			phase = 0;
			timer1counter = 0;
		}
		printf("\n,,,%n,%n,%n,%s",j,whatmouselicks,whatmouseshouldlick,sequence[trial_number][1]);
	}
	while(1){
	printf("end");
	}
}