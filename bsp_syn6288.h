#ifndef __BSP_SYN6288_H__
#define __BSP_SYN6288_H__

#include "user_config.h"


bool BSP_SYN6288_Init(void);
bool BSP_SYN6288_WaitBusy(u16 time_s);

void BSP_SYN6288_OutputVoice(u8 music_level,u8 music_type,u8 voice_level,u8 code_format,u8* data,u8 len);
void BSP_SYN6288_StopOutputVoice(void);

void BSP_SYN6288_EnterSleep(void);
void BSP_SYN6288_ExitSleep(void);



#endif

