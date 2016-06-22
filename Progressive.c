/*****************************************************

     Progressive Bonus.C
     Silicon Swamp Technology, 11/2001
     by Brian Merchant
     
     for RabbitSemiconductors JackRabbit Module
     Compile using Dynamic C (V 7.05)

     This program reads input pulses from external source
     and accumulates them into two Data buffers (A and B)
     and displays the amount on an led sign.
     It also has a setup mode to adjust features
     described below.

     1. Setup Mode (switch 1 on)
        A. Set Base Amount ($1.00 - $100,000.00)
        B. Set Contribution amount (1 - 100%)
        C.Set Input Weight (1 - 100%) - ratio of input
          put in buffer A or buffer B - i.e. 75/25 (A/B)

     03/20/2016 Edited source code to be readable in any text editor.

******************************************************/
/*****************************************************
               Hardware Connection Definition

     1. PC4 is TXD out to sign (must be INVERTED externally (7414))
     2. PC5 is RXD from sign (optional also needs external inversion)
     3. PB0 is pulse input from CONCENTRATOR
     4. HV1 is high current driver output to METER
	
******************************************************/
/*****************************************************
     The following I/O are defined on PORT B and PORT E
     
     PB0 = Pulse Input     (from external concentrator)
     PB2 = Next Switch            S1
     PB3 = Change Switch          S2
     PB4 = Advance Switch         S3
     PB5 = Setup Switch           S4  			
     PE1 = Meter Out

     S1 and S3 together for 4 seconds = Key Down
     			
******************************************************/
#define BINBUFSIZE 31
#define BOUTBUFSIZE 31

/*****************************************************
     Variable and Data Declarations		     ***/
	
     unsigned long Prog_Init;
     unsigned long LIMIT;                    //holds value of progressive data value limit
     unsigned long BASE;                     //holds base value
     unsigned int CONTRIB;
     unsigned int WEIGHT;
	
     int vswitch;                            // state of virtual switch controlled by button S1
     protected unsigned long data_A;         //variable to hold main counter
     protected unsigned long sub_data_A;     //used to calculate contribution value
     protected unsigned long data_B;         //variable to hold shadow counter (replaces main on key down)
     protected unsigned long sub_data_B;     //used to calculate contribution value
     protected unsigned long data_C;         //variable to number to count out to meter
     char send [50];                         //holds full message to send to sign
     char data_hex [15];                     //holds ascii converted value of data
     char string [50];
     int t;                                  //misc counter
		
/*****************************************************
     Function Definitions			     ***/
	
/********/
     void Update_Prog (void)                           //puts number on sign, uses global variable data_A
       {
          strcpy(send,"\x01\x62");                     //peace together final string to send
          ltoa(data_A,data_hex);                       //convert data to ascii
          strncat(send,data_hex,strlen(data_hex));
          strcat(send,"\x0D");
          serBwrFlush();
          serBopen(19200);
          serBputs(send);
          while (serBwrFree() != BOUTBUFSIZE) ;
          serBclose();
        }

//*****************************************************

main()
{
     unsigned long data_temp;
     unsigned long base_temp;
     unsigned int contrib_temp;
     unsigned int weight_temp;
     unsigned int hold;
     unsigned int hold_b;
     int A;
     int B;
     int C;
     int mode;
     int flag_s1;
     int flag_s2;
     int flag_s3;
     flag_s1 = 0;
     flag_s2 = 0;
     flag_s3 = 0;
     mode = 0;
	
     //data_A = 0;         //******** UNCOMMENT ONLY FOR INITALIZATION !!!!!!
     //data_B = 0;         //********
     //data_C = 0;         //********
	

     BitWrPortI(PEDDR, &PEDRShadow, 1, 1);       //set Port E bit 1 as output (for meter)

	
     LIMIT = 99999999;     //******** Set Progressive data value limit here !
     //BASE = 10000;       //******** Set Base Value
     //CONTRIB = 50;       //******** Set Contribution Ratio
     //WEIGHT = 90;        //******** Set Weight of Input
	
     WrPortI(SPCR, &SPCRShadow, 0x84);            // setup parallel port A as output
     WrPortI(PADR, &PADRShadow, 0xff);            // turn off all LED's
   
	
     vswitch=0;	                                   // initialize virtual switch as off
                                                  //
     if (BASE > LIMIT){BASE = 10000;}             //if not setup use defalts!!!!
     if (BASE < 10000){BASE = 10000;}		     //this initalizes vars on first use
     if (CONTRIB > 100){CONTRIB = 50;}
     if (WEIGHT > 100){WEIGHT = 90;}
     if (data_A > LIMIT){data_A = BASE;}
     if (data_B > LIMIT){data_B = BASE;}
     if (data_A < BASE){data_A = BASE;}
     if (data_B < BASE){data_B = BASE;}
     if (data_C > LIMIT){data_C = 0;}
     //if (hold < WEIGHT){hold = WEIGHT;}
     //if (hold > WEIGHT){hold = WEIGHT;}
     //if (sub_data_A > 100){sub_data_A = 0;}
     //if (sub_data_B > 100){sub_data_B = 0;}
     sub_data_A = 0;
     sub_data_B = 0;
     hold = WEIGHT;
     hold_b = 0;
       
     while (1) {                                  // endless loop
       BigLoopTop();                              // begin a big endless loop
	
     // First task will flash LED DS4 for 200ms once per second.
     costate {    
       BitWrPortI(PADR, &PADRShadow, 0, 3);       // turn LED on
       waitfor(DelayMs(200));                     // wait 200 milliseconds
       BitWrPortI(PADR, &PADRShadow, 1, 3);       // turn LED off
       waitfor(DelayMs(800));                     // wait 800 milliseconds
     }

          
     // Second task will debounce switch S1 and toggle virtual switch vswitch.
     // also check button 1 and toggle vswitch on or off
     costate {
       if (mode > 0)                              //if in setup mode skip other functions
         abort;
       if (BitRdPortI(PBDR, 0))
         abort;	                                   // if button not down skip out of costatement

       waitfor(DelayMs(10));                      // wait 50 ms

         if(BitRdPortI(PBDR,0))
           abort;                                 // if button not still down skip out
         
       vswitch = !vswitch;                        // toggle virtual switch since button was down 50 ms
       if (data_A < LIMIT)
         {
           hold++;
           if (hold == 101)
             {
               hold = WEIGHT;
               sub_data_A = sub_data_A + CONTRIB;
               sub_data_B = sub_data_B + 100;
               sub_data_B = sub_data_B - CONTRIB;
               if (sub_data_A > 99)
                 {
                   sub_data_A = sub_data_A - 100;
                   data_A++;
                 }
                 if (sub_data_B > 99)
                   {
                     sub_data_B = sub_data_B - 100;
                     data_B++;
                   }
						
             }
         }
         else if(data_A > LIMIT)
           data_A = LIMIT;
			
         while (1) {
           waitfor(BitRdPortI(PBDR, 0));          // wait for button to go up
           waitfor(DelayMs(10));                  // wait additional 10 milliseconds
           if (BitRdPortI(PBDR,0))
             break;                               // if button still up break out of while loop
          }//end of while
     } // end of costate
               
       // make led agree with vswitch if vswitch has changed
     if( (PADRShadow & 1) == vswitch)
       BitWrPortI(PADR, &PADRShadow, !vswitch, 0);
     
     costate {
       Update_Prog();
				
     } // end of costate

//***************************************************
//                    Get Switch Inputs 1-3
     //*** Switch 1
     costate {
       if (flag_s1 == 2)flag_s1 = 0;               //if it = 2 it has already been handled so reset it
         if (BitRdPortI(PBDR, 2))
           abort;			
       waitfor(DelayMs(50));	
       if(BitRdPortI(PBDR,2))
         abort;
       flag_s1=1;                                  //signal switch 1 made	
       while (1) {
         waitfor(BitRdPortI(PBDR, 2));	
         waitfor(DelayMs(50));		
         if (BitRdPortI(PBDR,2))
           break;			
       }//end of while
     } // end of costate
      
     //*** Switch 2			
     costate {
       if (flag_s2 == 2)flag_s2 = 0;		      //if it = 2 it has already been handled so reset it
         if (BitRdPortI(PBDR, 3))
           abort;			
       waitfor(DelayMs(50));	
       if(BitRdPortI(PBDR,3))
         abort;
       flag_s2=1;                                  //signal switch 2 made	
       while (1) {
         waitfor(BitRdPortI(PBDR, 3));	
         waitfor(DelayMs(50));		
         if (BitRdPortI(PBDR,3))
           break;			
       }//end of while
     } // end of costate

     //*** Switch 3
     costate {
       if (flag_s3 == 2)flag_s3 = 0;		     //if it = 2 it has already been handled so reset it
         if (BitRdPortI(PBDR, 4))
           abort;			
       waitfor(DelayMs(50));	
       if(BitRdPortI(PBDR,4))
         abort;
       flag_s3=1;                                 //signal switch 3 made	
       while (1) {
         waitfor(BitRdPortI(PBDR, 4));	
         waitfor(DelayMs(50));		
           if (BitRdPortI(PBDR,4))
           break;			
       }//end of while
     } // end of costate

     //*** Switch KEY DOWN
     costate {   	
       if (BitRdPortI(PBDR, 4))                   //if sw 3 and sw 1 together = KEY
         abort;			
       waitfor(DelayMs(50));	
       if(BitRdPortI(PBDR,4))
         abort;
       if (BitRdPortI(PBDR, 2))
         abort;			
       waitfor(DelayMs(50));	
       if(BitRdPortI(PBDR,2))
         abort;
       KEY:        	
       data_C = data_A;
       if (data_C > 100)data_C = data_C / 100;      //Thanks Larry!!!!!!!!
         data_A = data_B;
         data_B = BASE;           	
     } // end of costate

     costate{                                        //METER OUT		
       if (data_C == 0)abort;
       if (mode > 0)abort;
       BitWrPortI(PEDR, &PEDRShadow, 1, 1);
       waitfor(DelayMs(50));
       BitWrPortI(PEDR, &PEDRShadow, 0, 1);
       waitfor(DelayMs(50));
       data_C--;		
     }//end of costate
      			
//*********************** End of Get Switch Inputs				
      
//*****	Check for Setup Mode Switch
     costate{
       if (mode > 0)
         if (BitRdPortI(PBDR, 5))goto DONE;   		
           if (BitRdPortI(PBDR, 5))
             abort;                                   // if button not down skip out of costatement
           waitfor(DelayMs(50));			 // wait 50 ms
           if(BitRdPortI(PBDR,5))
             abort;                                   // if button not still down skip out
           if (mode > 0)goto MODE1;
           mode = 1;
           data_temp = data_A;
           base_temp = BASE;
           contrib_temp = CONTRIB * 100;
           weight_temp = WEIGHT * 100;
           data_A = 1;
           Update_Prog();
           A=1;
MODE1:
           while ( ! BitRdPortI(PBDR, 5))
             {
               if (flag_s2 == 1)
                 {
                   switch(A)
                     {
                       case 1:
                         data_A = base_temp;
                         Update_Prog();
                         waitfor(DelayMs(100));
                         data_A = 0;
                         Update_Prog();
                         waitfor(DelayMs(100));
                         if (flag_s1 == 1)
                           {
                             base_temp = base_temp + 10000;
                             if (base_temp > 1000000) base_temp = 10000;
                               flag_s1 = 2;
                           }
                         break;

                       case 2:
                         data_A = contrib_temp;
                         Update_Prog();
                         waitfor(DelayMs(100));
                         data_A = 0;
                         Update_Prog();
                         waitfor(DelayMs(100));
                         if (flag_s1 == 1)
                           {
                             contrib_temp = contrib_temp + 100;
                             if (contrib_temp > 10000) contrib_temp = 100;
                               flag_s1 = 2;
                           }
                         break;

                        case 3:
                          data_A = weight_temp;
                          Update_Prog();
                          waitfor(DelayMs(100));
                          data_A = 0;
                          Update_Prog();
                          waitfor(DelayMs(100));
                          if (flag_s1 == 1)
                            {
                              weight_temp = weight_temp + 100;
                              if (weight_temp > 10000) weight_temp = 100;
                              flag_s1 = 2;
                             }
                           break;
										
                         case 4:
                           if (flag_s2 == 1)
                             {
                               base_temp = 10000;
                               contrib_temp = 5000;
                               weight_temp = 9000;
                               data_temp = base_temp;
                               data_B = base_temp;
                               data_C = 0;
                               sub_data_A = 0;
                               sub_data_B = 0;
                               hold = WEIGHT;
                               flag_s2 = 2;
                               for (B=10;B>0;B--)
                                 {
                                   data_A = B;
                                   Update_Prog();
                                   waitfor(DelayMs(100));
                                   A = 1;
                                 }
                             }
                           break;
                           default:
                             break;			
                         }		
                     }
                     if (flag_s3 == 1)
                       {
                         if (A>3) A=0;		
                         A++;
                         data_A = A;
                         Update_Prog();
                         flag_s3=2;
                         flag_s2=2;
                       }
                     else break;
             }//end of while loop
       if (BitRdPortI(PBDR, 5))goto DONE;
       if (mode > 0)goto RETEST;
DONE:                                               //save yer data and get out of dodge!
       mode = 0;
       flag_s3=2;
       flag_s2=2;
       flag_s1=2;
       data_A = data_temp;
       BASE = base_temp;
       CONTRIB = contrib_temp / 100;
       WEIGHT = weight_temp / 100;
       Update_Prog();
					
RETEST:				
     } // end of costate
	runwatch();	
   }  // end of Big Loop
	
}  // end of main

