/**********************************************************
*�ļ���:MS82F_WPWM_TEST.C
*����:MS82Fxx02 CCP1ģ��PWM�������
*�����ͺ�:MS82F1402A
*����:�ڲ�RC 4MHz 2T
*���Ŷ���:
*                 ----------------
*  VDD-----------|1(VDD)   (VSS)14|------------GND
*  SSW-----------|2(PA7)   (PA0)13|-------------NC
*  PSW-----------|3(PA6)   (PA1)12|---------BATCHK
*  NC------------|4(PA4)   (PA2)11|----------DEBUG
*  PWM-----------|5(PC3)   (PA3)10|-------------TX-9600
*  SCL-----------|6(PC2)   (PC0)09|-------------NC
*  SDA-----------|7(PC4)   (PC1)08|------------LSW
*                 ----------------
*                 MS82F1402A SOP14
*˵��:����˵
**********************************************************/
#include "syscfg.h"
#include "MS82Fxx02.h"
#include "sysdef.h"
#include "EPWM.h"
#include "mpu6050.h"
#include "uart.h"
#include "other.h"

//#define	ENDEBUG	1

#ifdef ENDEBUG
//���ڵ��Բ���
unsigned char str[7];
#endif
//PWM����
volatile unsigned char pwmPer=0;
//��������ֵ
//#define N=10
int Z10[N];
int z,zz;
#define SMS_SLEEP	0
#define SMS_INIT	1
#define SMS_RUN		2
//״̬������
volatile unsigned char SMS=0;


void zz2pwm(int zz1);
/*====================================================
*������:interrupt ISR
*����:�жϷ�����
*�������:��
*���ز���:��
====================================================*/
void interrupt ISR(void)
{
	if(T0IE&&T0IF)
	{
		T0IF = 0;
		Uart_TX_Int();
	}
    IOCA_int();
}
/*====================================================
*������:DEVICE_INIT
*����:�ϵ�������ʼ��
*�������:��
*���ز���:��
====================================================*/
void DEVICE_INIT(void)
{
	OSCCON = 0B01110001;   //Bit7    >>>  LFMOD=0 >>> WDT����Ƶ��=32KHz
                           //Bit6:4 >>> IRCF[2:0]=111 >>> �ڲ�RCƵ��=16MHz
                           //Bit0   >>> SCS=1      >>> ϵͳʱ��ѡ��Ϊ�ڲ�����
	INTCON = 0B00000000; //�ݽ�ֹ�����ж�
	PIE1 = 0B00000000;
	PIR1 = 0B00000000;
	PORTA = 0B00000000;
	TRISA = 0B00000000;  //����PORTA�ڶ�Ϊ�����
	WPUA = 0B00000000;
	PORTC = 0b00000000;
	TRISC = 0B00000000;  //����PORTAC�ڶ�Ϊ�����
	WPUC = 0B00000000;
	OPTION = 0B00000000;
}

//=======================MainRoutine======================
void main(void)
{
#asm
	MOVLW		0x07			// ϵͳ���ò�����ɾ�����޸�
	MOVWF		0x19			// ϵͳ���ò�����ɾ�����޸�
#endasm
	uchar i=0;
	DEVICE_INIT();
	key_init();
	SMS = SMS_SLEEP;
	do{
		switch(SMS)
		{
			case SMS_SLEEP:
			#ifdef	ENDEBUG	
				UART_TX_init();	
				GIE =1;			
				UART_TX_print("\r\n --Sleep\r\n");
			#endif
				pwmPer=0;
				SET_EPWM_TO(pwmPer);
				SET_EPWM_OFF();
				goto_sleep();
				GIE =1;
				SLEEP();
				CLRWDT();
				key_init();
				key_power_time=0;
				for(i=0;i<10;i++){
					__delay_ms(10);
					key_scan();
					if(key_power_time>5){
						SMS= SMS_INIT;
						break;
					}
				}
				CLRWDT();
			break;
			
			case SMS_INIT:
				SET_EPWM_INIT();			//��ʼ��pwm�ӿ� 
				MPU_Init();					//��ʼ��MPU6050
	#ifdef	ENDEBUG			
				UART_TX_init();			
				UART_TX_print("\r\n --wakup\r\n");
	#endif	
				key_init();					//������ʼ��
				key_power_time=10;			//��ʼֵ
				GIE = 1;  					//�������ж�
				__delay_ms(100);			//�ȴ�sensor�ȶ�
	#ifdef	ENDEBUG				
				UART_TX_print("\r\nMS81Fxx02 UART-9600-8-N-1\r\n");
	#endif			
				pwmPer=0;   
				SET_EPWM_TO(pwmPer);
				SMS = SMS_RUN;
				CLRWDT();
			break;
				
			case SMS_RUN:
				while(1){	
					for(i=0;i<N;i++)
                    {
					  MPU_Get_Gyr_Z(&z);  // This will update x, y, and z with new values    
					  Z10[i]=z;
					  key_scan_H();
					  CLRWDT();
					  __delay_ms(5);
					}
					if(key_power_time>240)	
                    { 
                    	SMS=SMS_SLEEP;
                        break;
                    }
					zz=filter(Z10);
	#ifdef	ENDEBUG			
    				//zz=z;	
					int2str(str,zz);
					UART_TX_print(str);
					UART_TX_print("-");
	#endif				
					zz2pwm(zz);
					SET_EPWM_TO(pwmPer);
	#ifdef	ENDEBUG			
    				//zz=z;	
					int2str(str,pwmPer);
					UART_TX_print(str);
					UART_TX_print("\r\n");
	#endif	
				}
			break;
			
			default: 
				SMS=SMS_SLEEP;
				CLRWDT();
			break;
		}
	}while(1);
}



/*====================================================
*������:zz2pwm
*����:ת�����ٶȵ�����ռ�ձ�
*�������:���ٶ�
*���ز���:��
====================================================*/
void zz2pwm(int zz1){
  unsigned int a,b,c=0;
  if(zz1>=MID_VALUE){
    a=zz1-MID_VALUE;
    c=1;
  }else{
    a=MID_VALUE-zz1;
    c=0;
  }
  b=a>>6; //(0~7)
  if(c<1){
    if(pwmPer<=246) pwmPer+=b;
    }
  else {
    if(pwmPer>b) pwmPer-=b;
    }
  return;
}