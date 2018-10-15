#include "sys.h"
#include "usart.h"
#include "delay.h"
#include "car.h"
#include "led.h"
#include "lanya.h"
////////////////////////////////////////////////////////////////////////////////// 	 

 
#if EN_USART2_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART2_RX_BUF[USART2_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART2_RX_STA=0;       //接收状态标记	  
  
void uart2_init(u32 bound)
{
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);
	//USART1_TX   GPIOA.2
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.9
   
  //USART1_RX	  GPIOA.3初始化
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;//PA10
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.10  

  //Usart1 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
   //USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

  USART_Init(USART2, &USART_InitStructure); //初始化串口1
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启串口接受中断
  USART_Cmd(USART2, ENABLE);                    //使能串口1 

}


void USART2_IRQHandler(void)                	//串口1中断服务程序
	{
	u8 Res;
#if SYSTEM_SUPPORT_OS 		//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
		{
		Res =USART_ReceiveData(USART2);	//读取接收到的数据
		
		if((USART2_RX_STA&0x8000)==0)//接收未完成
			{
			if(USART2_RX_STA&0x4000)//接收到了0x0d
				{
				if(Res!=0x0a)USART2_RX_STA=0;//接收错误,重新开始
				else USART2_RX_STA|=0x8000;	//接收完成了 
				}
			else //还没收到0X0D
				{	
				if(Res==0x0d)USART2_RX_STA|=0x4000;
				else
					{
					USART2_RX_BUF[USART2_RX_STA&0X3FFF]=Res ;
					USART2_RX_STA++;
					if(USART2_RX_STA>(USART2_REC_LEN-1))USART2_RX_STA=0;//接收数据错误,重新开始接收	  
					}		 
				}
			}   		 
     } 
#if SYSTEM_SUPPORT_OS 	//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntExit();  											 
#endif
} 

void USART2_rx(void)
{
	u8 t;
	u16 len;
	if(USART2_RX_STA&0x8000)
		{					   
			len=USART2_RX_STA&0x3fff;    //获取数据的长度
	//		printf("\r\n您发送的消息为:\r\n\r\n");
			for(t=0;t<len ;t++)
			{
				USART_SendData(USART2, USART2_RX_BUF[t]);   //发送获取到的数据到串口，两个参数都为全局变量
				while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);  //等待发送完成，发送完成后才能循环到发送下个数据
			}
			check2(len); 	
			printf("\r\n\r\n");
			USART2_RX_STA=0;
		}		
		delay_ms(500);
	
}

void check2(u16 len)
{
	u16 t=0;
	
		if(USART2_RX_BUF[t]==0xc7&&USART2_RX_BUF[t+1]==0xb0&&USART2_RX_BUF[t+2]==0xBD&&USART2_RX_BUF[t+3]==0xf8)
		{
			go_speed(900,0,450);
		}
  	 if(USART2_RX_BUF[t]==0xBA&&USART2_RX_BUF[t+1]==0xF3&&USART2_RX_BUF[t+2]==0xCD&&USART2_RX_BUF[t+3]==0xCB)
		{
			back_speed(900,0,450);
		}
		 if(USART2_RX_BUF[t]==0xD7&&USART2_RX_BUF[t+1]==0xF3&&USART2_RX_BUF[t+2]==0xD7&&USART2_RX_BUF[t+3]==0xAA)
		{
			left_speed(900,0,250);
		}
		 if(USART2_RX_BUF[t]==0xd3&&USART2_RX_BUF[t+1]==0xd2&&USART2_RX_BUF[t+2]==0xd7&&USART2_RX_BUF[t+3]==0xaa)
		{
			right_speed(900,0,250);
		}
		 if(USART2_RX_BUF[t]==0xcd&&USART2_RX_BUF[t+1]==0xa3&&USART2_RX_BUF[t+2]==0xd6&&USART2_RX_BUF[t+3]==0xb9)
		{
			stop();
		}
		if(USART2_RX_BUF[t]==0xd2&&USART2_RX_BUF[t+1]==0xbb&&USART2_RX_BUF[t+2]==0xb5&&USART2_RX_BUF[t+3]==0xb5&&USART2_RX_BUF[t+4]==0xc7&&USART2_RX_BUF[t+5]==0xb0&&USART2_RX_BUF[t+6]==0xbd&&USART2_RX_BUF[t+7]==0xf8)
		{
			go_speed(900,0,350);   //一档前进
		}
		if(USART2_RX_BUF[t]==0xb6&&USART2_RX_BUF[t+1]==0xfe&&USART2_RX_BUF[t+2]==0xb5&&USART2_RX_BUF[t+3]==0xb5&&USART2_RX_BUF[t+4]==0xc7&&USART2_RX_BUF[t+5]==0xb0&&USART2_RX_BUF[t+6]==0xbd&&USART2_RX_BUF[t+7]==0xf8)
		{
			go_speed(900,0,250);   //二档前进
		}
		if(USART2_RX_BUF[t]==0xc8&&USART2_RX_BUF[t+1]==0xfd&&USART2_RX_BUF[t+2]==0xb5&&USART2_RX_BUF[t+3]==0xb5&&USART2_RX_BUF[t+4]==0xc7&&USART2_RX_BUF[t+5]==0xb0&&USART2_RX_BUF[t+6]==0xbd&&USART2_RX_BUF[t+7]==0xf8)
		{
			go_speed(900,0,50);   //三档前进
		}
		if(USART2_RX_BUF[t]==0xd2&&USART2_RX_BUF[t+1]==0xbb&&USART2_RX_BUF[t+2]==0xb5&&USART2_RX_BUF[t+3]==0xb5&&USART2_RX_BUF[t+4]==0xba&&USART2_RX_BUF[t+5]==0xf3&&USART2_RX_BUF[t+6]==0xcd&&USART2_RX_BUF[t+7]==0xcb)
		{
			back_speed(900,0,350);   //一档后退
		}
		if(USART2_RX_BUF[t]==0xb6&&USART2_RX_BUF[t+1]==0xfe&&USART2_RX_BUF[t+2]==0xb5&&USART2_RX_BUF[t+3]==0xb5&&USART2_RX_BUF[t+4]==0xba&&USART2_RX_BUF[t+5]==0xf3&&USART2_RX_BUF[t+6]==0xcd&&USART2_RX_BUF[t+7]==0xcb)
		{
			back_speed(900,0,250);   //二档后退
		}
		if(USART2_RX_BUF[t]==0xc8&&USART2_RX_BUF[t+1]==0xfd&&USART2_RX_BUF[t+2]==0xb5&&USART2_RX_BUF[t+3]==0xb5&&USART2_RX_BUF[t+4]==0xba&&USART2_RX_BUF[t+5]==0xf3&&USART2_RX_BUF[t+6]==0xcd&&USART2_RX_BUF[t+7]==0xcb)
		{
			back_speed(900,0,50);    //三档后退
		}		
		
		
	
}

#endif	

