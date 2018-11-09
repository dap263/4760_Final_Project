#ifndef EDITED_UART_H
#define EDITED_UART_H

#include <plib.h>
#include <pt_cornell_1_2_3.h>

char PT_terminate_char = '\n';
char PT_terminate_count;
// terminate time default million seconds
int PT_terminate_time = 1000000000 ;
// timeout return value
int PT_timeout = 0; 

// system time updated in TIMER5 ISR below
volatile unsigned int time_tick_millsec ;

int PT_GetMachineBuffer1(struct pt *pt, char* buffer)
{
    static char character;
    static unsigned int num_char, start_time;
    // mark the beginnning of the input thread
    PT_BEGIN(pt);
    
    // actual number received
    num_char = 0;
    //record milliseconds for timeout calculation
    start_time = time_tick_millsec ;
    // clear timeout flag
    PT_timeout = 0;
    // clear input buffer
    memset(buffer, 0, max_chars);
    
    while(num_char < max_chars)
    {
        // get the character
        // yield until there is a valid character so that other
        // threads can execute
        PT_YIELD_UNTIL(pt, 
                UARTReceivedDataIsAvailable(UART1) || 
                ((PT_terminate_time>0) && (time_tick_millsec >= PT_terminate_time+start_time)));
       // grab the character from the uart buffer
        character = UARTGetDataByte(UART1);
        
        // Terminate on character match
        if ((character>0) && (character == PT_terminate_char)) {
            buffer[num_char] = 0; // zero terminate the string
            // and leave the while loop
            break;
        }    
        // Terminate on count
        else if ( ((PT_terminate_count>0) && (num_char+1 >= PT_terminate_count))){
            // record the last character
            buffer[num_char++] = character ; 
            // and terminate
            buffer[num_char] = 0; // zero terminate the string
            // and leave the while loop
            break;
        }
        // terminate on timeout
        else if ((PT_terminate_time>0) && (time_tick_millsec >= PT_terminate_time+start_time)){
            // set the timeout flag
            PT_timeout = 1;
            // clear (probably invalid) input buffer
            memset(buffer, 0, max_chars);
            // and  leave the while loop
            break ;
        }
        // continue recording input characters
        else {
            buffer[num_char++] = character ;  
        }
    } //end while(num_char < max_size)
    
    // kill this input thread, to allow spawning thread to execute
    PT_EXIT(pt);
    // and indicate the end of the thread
    PT_END(pt);
}

int PT_GetMachineBuffer2(struct pt *pt, char* buffer)
{
    static char character;
    static unsigned int num_char, start_time;
    // mark the beginnning of the input thread
    PT_BEGIN(pt);
    
    // actual number received
    num_char = 0;
    //record milliseconds for timeout calculation
    start_time = time_tick_millsec ;
    // clear timeout flag
    PT_timeout = 0;
    // clear input buffer
    memset(buffer, 0, max_chars);
    
    while(num_char < max_chars)
    {
        // get the character
        // yield until there is a valid character so that other
        // threads can execute
        PT_YIELD_UNTIL(pt, 
                UARTReceivedDataIsAvailable(UART2) || 
                ((PT_terminate_time>0) && (time_tick_millsec >= PT_terminate_time+start_time)));
       // grab the character from the uart buffer
        character = UARTGetDataByte(UART2);
        
        // Terminate on character match
        if ((character>0) && (character == PT_terminate_char)) {
            buffer[num_char] = 0; // zero terminate the string
            // and leave the while loop
            break;
        }    
        // Terminate on count
        else if ( ((PT_terminate_count>0) && (num_char+1 >= PT_terminate_count))){
            // record the last character
            buffer[num_char++] = character ; 
            // and terminate
            buffer[num_char] = 0; // zero terminate the string
            // and leave the while loop
            break;
        }
        // terminate on timeout
        else if ((PT_terminate_time>0) && (time_tick_millsec >= PT_terminate_time+start_time)){
            // set the timeout flag
            PT_timeout = 1;
            // clear (probably invalid) input buffer
            memset(buffer, 0, max_chars);
            // and  leave the while loop
            break ;
        }
        // continue recording input characters
        else {
            buffer[num_char++] = character ;  
        }
    } //end while(num_char < max_size)
    
    // kill this input thread, to allow spawning thread to execute
    PT_EXIT(pt);
    // and indicate the end of the thread
    PT_END(pt);
}

//====================================================================
// === DMA send string to the UART1 ==================================
// int PT_DMA_PutSerialBuffer1(struct pt *pt, char* buffer)
// {
//     PT_BEGIN(pt);
//     //mPORTBSetBits(BIT_0);
//     // check for null string
//     if (buffer[0]==0)PT_EXIT(pt);
//     // sent the first character
//     PT_YIELD_UNTIL(pt, UARTTransmitterIsReady(UART1));
//     UARTSendDataByte(UART1, buffer[0]);
//     //DmaChnStartTxfer(DMA_CHANNEL1, DMA_WAIT_NOT, 0);
//     // start the DMA
//     DmaChnEnable(DMA_CHANNEL1);
//     // wait for DMA done
//     //mPORTBClearBits(BIT_0);
//     PT_YIELD_UNTIL(pt, DmaChnGetEvFlags(DMA_CHANNEL1) & DMA_EV_BLOCK_DONE);
//     //wait until the transmit buffer is empty
//     PT_YIELD_UNTIL(pt, U1STA&0x100);
    
//     // kill this output thread, to allow spawning thread to execute
//     PT_EXIT(pt);
//     // and indicate the end of the thread
//     PT_END(pt);
// }

//====================================================================
// === DMA send string to the UART2 ==================================
int PT_DMA_PutSerialBuffer2(struct pt *pt, char* buffer)
{
    PT_BEGIN(pt);
    //mPORTBSetBits(BIT_0);
    // check for null string
    if (buffer[0]==0)PT_EXIT(pt);
    // sent the first character
    PT_YIELD_UNTIL(pt, UARTTransmitterIsReady(UART2));
    UARTSendDataByte(UART2, PT_send_buffer[0]);
    //DmaChnStartTxfer(DMA_CHANNEL1, DMA_WAIT_NOT, 0);
    // start the DMA
    DmaChnEnable(DMA_CHANNEL1);
    // wait for DMA done
    //mPORTBClearBits(BIT_0);
    PT_YIELD_UNTIL(pt, DmaChnGetEvFlags(DMA_CHANNEL1) & DMA_EV_BLOCK_DONE);
    //wait until the transmit buffer is empty
    PT_YIELD_UNTIL(pt, U2STA&0x100);
    
    // kill this output thread, to allow spawning thread to execute
    PT_EXIT(pt);
    // and indicate the end of the thread
    PT_END(pt);
}

#define UART1_RX_PIN // The RX pin must be one of the Group 3 input pins: RPA2, RPB6, RPA4, RPB13, RPB2
#define UART1_TX_PIN // The TX pin must be one of the Group 1 output pins: RPA0, RPB3, RPB4, RPB15, RPB7
#define UART1_BAUD
#define UART2_RX_PIN RPA1 // The RX pin must be one of the Group 2 input pins: RPA1, RPB1, RPB5, RPB8, RPB11
#define UART2_TX_PIN RPB10 // The TX pin must be one of the Group 4 output pins: RPA3, RPB0, RPB9, RPB10, RPB14 
#define UART2_BAUD

void init_UART1 (void) {
    PPSInput(3, U1RX, UART1_RX_PIN); // Assign U1RX to UART1_RX_PIN
    PPSOutput(1, UART1_TX_PIN, U1TX); // Assign U2TX to UART1_TX_PIN
    UARTConfigure(UART1, UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetLineControl(UART1, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(UART1, pb_clock, UART1_BAUD);
    UARTEnable(UART1, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
}

void init_UART2 (void) {
    PPSInput (2, U2RX, UART2_RX_PIN); //Assign U2RX to UART2_RX_PIN
    PPSOutput(4, UART2_TX_PIN, U2TX); //Assign U2TX to UART2_TX_PIN 
    UARTConfigure(UART2, UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetLineControl(UART2, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(UART2, pb_clock, BAUDRATE);
    UARTEnable(UART2, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
}

void PT_setup_2UART (void)
{
  // Configure the device for maximum performance but do not change the PBDIV
    // Given the options, this function will change the flash wait states, RAM
    // wait state and enable prefetch cache but will not change the PBDIV.
    // The PBDIV value is already set via the pragma FPBDIV option above..
    SYSTEMConfig(sys_clock, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

  ANSELA =0; //make sure analog is cleared
  ANSELB =0;
  
  // === init both uarts ===================
    init_UART1();
    init_UART2();
 
  // Feel free to comment this out
//   clrscr();
//   home();
  // reverse video control codes
//   normal_text;
//   rev_text ;
//   printf("...protothreads 1_2_3 07/10/18...");
//   normal_text ;
  // === set up DMA for UART output =========
  // configure the channel and enable end-on-match
  DmaChnOpen(DMA_CHANNEL1, DMA_CHN_PRI2, DMA_OPEN_MATCH);
  // trigger a byte everytime the UART is empty
  DmaChnSetEventControl(DMA_CHANNEL1, DMA_EV_START_IRQ_EN|DMA_EV_MATCH_EN|DMA_EV_START_IRQ(_UART2_TX_IRQ));
  // source and destination
  DmaChnSetTxfer(DMA_CHANNEL1, PT_send_buffer+1, (void*)&U2TXREG, max_chars, 1, 1);
  // signal when done
  DmaChnSetEvEnableFlags(DMA_CHANNEL1, DMA_EV_BLOCK_DONE);
  // set null as ending character (of a string)
  DmaChnSetMatchPattern(DMA_CHANNEL1, 0x00);
  
  // ===Set up timer5 ======================
  // timer 5: on,  interrupts, internal clock, 
  // set up to count millsec
  OpenTimer5(T5_ON  | T5_SOURCE_INT | T5_PS_1_1 , pb_clock/1000);
  // set up the timer interrupt with a priority of 2
  ConfigIntTimer5(T5_INT_ON | T5_INT_PRIOR_2);
  mT5ClearIntFlag(); // and clear the interrupt flag
  // zero the system time tick
  time_tick_millsec = 0;

}

#endif
