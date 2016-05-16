#include "bsp_syn6288.h"
#include "hal_usart3.h"

typedef struct
{
	u8 send_buf[200];		//发送数据缓冲区
	
	
}Syn6288_Type;
Syn6288_Type syn6288;

/*
需要移植的代码：
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//接口函数-毫秒级延时函数
#define BSP_SYN6288_DelayMs_Port(ms)		CoTickDelay(ms)

//接口函数-发送数据函数(串口发送指定长度的数据)
#define BSP_SYN6288_SendData_Port(p_data,len)		HAL_USART3_SendData(p_data,len)

//接口函数-读取BUSY引脚点平状态
#define BSP_SYN6288_ReadBusy_Port()		GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_15)

//SYN6288引脚初始化
void BSP_SYN6288_Init_Port(void)
{
	//串口初始化
	HAL_USART3_Init(9600);
	
	//BUSY引脚初始化(高电平-忙  低电平-不忙)
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;		//输入
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;	//开漏
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;	//下拉
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 ************************************************************
 *  名称：	BSP_SYN6288_Init()
 *  功能：	初始化
 *	输入：  无
 *	输出：  true-成功  false-失败
 ************************************************************
*/
bool BSP_SYN6288_Init(void)
{
	//IO口初始化
	BSP_SYN6288_Init_Port();
	
	u8 i=0;
	while(BSP_SYN6288_ReadBusy_Port() == 1)	//等待芯片忙
	{
		BSP_SYN6288_DelayMs_Port(100);
		i++;
		if(i > 10)	return false;
	}
	
	BSP_SYN6288_ExitSleep();
	
	return true;
}

/*
 ************************************************************
 *  名称：	BSP_SYN6288_WaitBusy()
 *  功能：	等待繁忙
 *	输入：  time_s-等待时间S
 *	输出：  true-成功  false-失败
 ************************************************************
*/
bool BSP_SYN6288_WaitBusy(u16 time_s)
{
	if(time_s == 0)
	{
		if(BSP_SYN6288_ReadBusy_Port() == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		while(time_s--)
		{
			BSP_SYN6288_DelayMs_Port(1000);
			
			if(BSP_SYN6288_ReadBusy_Port() == 0)
			{
				return true;
			}
		}
		return false;
	}
}

/*
 ************************************************************
 *  名称：	BSP_SYN6288_OutputVoice()
 *  功能：	输出语音
 *	输入：  music_level-背景音乐音量
						music_type-背景音乐   0:无背景音乐 1~15:背景音乐1~15
						voice_level-语音文本音量
						code_format-编码方式  0:GB2312 1:GBK 2:BIG5 3:UNICODE
						data-输出的语音数据
						len-数据长度(小于190)
 *	输出：  无
 ************************************************************
*/
void BSP_SYN6288_OutputVoice(u8 music_level,u8 music_type,u8 voice_level,u8 code_format,u8* data,u8 len)
{	
	if(len > 190)	len = 190;
	
	syn6288.send_buf[0] = 0xFD;		//帧头
	
	syn6288.send_buf[1] = 0;			//数据区长度
	syn6288.send_buf[2] = 0;
	
	//--------------数据区--------------
	syn6288.send_buf[3] = 0x01;		//命令字节
	
	//命令参数字节(高五位:背景音乐  低三位:编码格式)
	syn6288.send_buf[4] = ((music_type<<5) + code_format);
	
	//语音输出模式设置-语音音量[v?]/背景音乐音量[m?]
	char buf[10];
	u8 control_cnum1=0;
	u8 control_cnum2=0;
	
	control_cnum1 = sprintf(buf,"[m%d]",music_level);	//背景音乐音量
	for(u8 i=0;i<control_cnum1;i++)
	{
		syn6288.send_buf[5+i] = buf[i];
	}
	control_cnum2 = sprintf(buf,"[v%d]",voice_level);	//语音文本音量
	for(u8 i=0;i<control_cnum2;i++)
	{
		syn6288.send_buf[5+control_cnum1+i] = buf[i];
	}
	
	//待发送文本
	for(u8 i=0;i<len;i++)
	{
		syn6288.send_buf[5+control_cnum1+control_cnum2+i] = data[i];
	}
	
	u16 temp = control_cnum1+control_cnum2+len+3;
	syn6288.send_buf[1] = ((temp>>8)&0xFF);			//数据区长度
	syn6288.send_buf[2] = (temp&0xFF);
	
	
	//异或校验
	u8 check=0;
	for(u8 i=0;i<(len+5+control_cnum1+control_cnum2);i++)
	{
		check ^= syn6288.send_buf[i];
	}
	syn6288.send_buf[5+control_cnum1+control_cnum2+len] = check;
	
	BSP_SYN6288_SendData_Port(syn6288.send_buf,len+6+control_cnum1+control_cnum2);	//发送数据
}

/*
 ************************************************************
 *  名称：	BSP_SYN6288_StopOutputVoice()
 *  功能：	停止合成语音
 *	输入：  无
 *	输出：  true-成功  false-失败
 ************************************************************
*/
void BSP_SYN6288_StopOutputVoice(void)
{
	syn6288.send_buf[0] = 0xFD;		//帧头
	syn6288.send_buf[1] = 0x00;		//数据区长度
	syn6288.send_buf[2] = 0x02;		
	
	syn6288.send_buf[3] = 0x02;	//命令字
	
	syn6288.send_buf[4] = 0xFD;	//异或校验
	
	BSP_SYN6288_SendData_Port(syn6288.send_buf,5);	//发送数据
}

/*
 ************************************************************
 *  名称：	BSP_SYN6288_EnterSleep()
 *  功能：	进入休眠模式
 *	输入：  无
 *	输出：  true-成功  false-失败
 ************************************************************
*/
void BSP_SYN6288_EnterSleep(void)
{
	syn6288.send_buf[0] = 0xFD;		
	syn6288.send_buf[1] = 0x00;
	syn6288.send_buf[2] = 0x02;		
	
	syn6288.send_buf[3] = 0x88;
	
	syn6288.send_buf[4] = 0x77;
	
	BSP_SYN6288_SendData_Port(syn6288.send_buf,5);	//发送数据
}

/*
 ************************************************************
 *  名称：	BSP_SYN6288_ExitSleep()
 *  功能：	退出休眠模式
 *	输入：  无
 *	输出：  true-成功  false-失败
 ************************************************************
*/
void BSP_SYN6288_ExitSleep(void)
{
	syn6288.send_buf[0] = 0xFD;		
	syn6288.send_buf[1] = 0x00;
	syn6288.send_buf[2] = 0x02;		
	
	syn6288.send_buf[3] = 0x21;
	
	syn6288.send_buf[4] = 0xDE;
	
	BSP_SYN6288_SendData_Port(syn6288.send_buf,5);	//发送数据
	
	BSP_SYN6288_DelayMs_Port(200);
	BSP_SYN6288_SendData_Port(syn6288.send_buf,5);	//防止10S内不发数据再次进入休眠模式
	BSP_SYN6288_DelayMs_Port(200);
}
























