/**********************************************************
*�ļ���:MS82F_TIMER0_UART_TX.C
*����:MS82Fxx02��TIMER0ʵ�ִ��ڷ���
*�����ͺ�:MS82F1402A
*����:�ڲ�RC 16MHz 2T
*���Ŷ���:
*                 ----------------
*  VDD-----------|1(VDD)   (VSS)14|------------GND
*  TX------------|2(PA7)   (PA0)13|-------------NC
*  NC------------|3(PA6)   (PA1)12|-------------NC
*  NC------------|4(PA4)   (PA2)11|-------------NC
*  NC------------|5(PC3)   (PA3)10|-------------NC
*  NC------------|6(PC2)   (PC0)09|-------------NC
*  NC------------|7(PC4)   (PC1)08|-------------NC
*                 ----------------
*                 MS82F1402A SOP14
*˵��:����TIMER0�ķ�Ƶ��Ϊ1:4���ڲ�����Ϊ4MHz(4T)��TIMER0��ֵΪ6��
      ��TIMER0���ʱ��Ϊ1ms�����RA0ÿ1ms��תһ�Ρ�
**********************************************************/
#include "syscfg.h"
#include "MS82Fxx02.h"
#include "uart.h"
//------------------------------------------------------------------------------
// Conditions for 9600 Baud SW UART, TIMER_CLK = 2MHz
//------------------------------------------------------------------------------
#define	PRESCALER_1_2		0x01
#define UART_TBIT           200//(2000000 / 9600)


#define	TIMER_OFFSET_INC	(TMR0=255-UART_TBIT)
#define TIMER_INT_CLOSE		(T0IE=0)
#define TIMER_INT_OPEN		(T0IE=1)
#define	UART_TXD			RA3	
#define	DEBUG_PIN			RA2	


//------------------------------------------------------------------------------
// Global variables used for full-duplex UART communication
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Function configures Timer_T0 for UART TX operation
//------------------------------------------------------------------------------
void UART_TX_init(void)
{
	TRISA3 = 0;
	RA3 = 1;	
    UART_TXD = 1;                          // Set TXD Idle as Mark = '1'
    OPTION &= 0b11111000;
    OPTION |= PRESCALER_1_2;			   //fosc/2,t0 rate.
    T0IF=0;
    T0IE=0;
	bSendCharEnd=1;
}
//------------------------------------------------------------------------------
// Outputs one byte using the UART_TX_char
//------------------------------------------------------------------------------
void UART_TX_char(unsigned char byte)
{
    while (!bSendCharEnd);                 	// Ensure last char got TX'd
	bSendCharEnd=0;
    txData = byte;                          // Load global variable
    txData |= 0x100;                        // Add mark stop bit to TXData
    txData <<= 1;                           // Add space start bit
    TIMER_OFFSET_INC;                    	// One bit time till first bit
	TIMER_INT_OPEN;
}


//------------------------------------------------------------------------------
// Prints a string over using the Timer_A UART
//------------------------------------------------------------------------------
void UART_TX_print(unsigned char *string)
{
    while (*string) {
		CLRWDT();
        UART_TX_char(*string++);
    }
}

/*
	uart receive function
	
*/
void Uart_TX_Int(void)
{

    static unsigned char txBitCnt = 10;
	DEBUG_PIN=~DEBUG_PIN;
    TIMER_OFFSET_INC;                    // Add Offset to CCRx
    if (txBitCnt == 0) {                    // All bits TXed?
        TIMER_INT_CLOSE;                   // All bits TXed, disable interrupt
        txBitCnt = 10;                      // Re-load bit counter
		bSendCharEnd=1;
    }
    else {
        if (txData & 0x01) {
          UART_TXD = 1;              // TX Mark '1'
        }
        else {
          UART_TXD = 0;               // TX Space '0'
        }
        txData >>= 1;
        txBitCnt--;
    }
}
/*
 int to str with signed
 len(s)>=7
*/

void int2str(unsigned char *s,int temp_data)
{
	if(temp_data<0){
	temp_data=-temp_data;
    *s='-';
	}
	else *s=' ';
    if(temp_data>10000){
	    *++s =temp_data/10000+0x30;
	    temp_data=temp_data%10000;     //???��????
    }
    if(temp_data>1000){
    	*++s =temp_data/1000+0x30;
    	temp_data=temp_data%1000;     //ȡ������  
    }  
    if(temp_data>100){    
    	*++s =temp_data/100+0x30;
    	temp_data=temp_data%100;     //ȡ������
    }
    if(temp_data>10){
    	*++s =temp_data/10+0x30;
    	temp_data=temp_data%10;      //ȡ������
    }
    *++s =temp_data+0x30;
    *++s=0; 	
}