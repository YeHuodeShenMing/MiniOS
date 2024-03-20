#ifndef MINIOS_TIME_H
#define MINIOS_TIME_H

#include "type.h"
#define TIMER0          0X40            //counter0端口
#define TIMER_MODE      0x43            //模式控制寄存器端口
#define RATE_GENERATOR  0X34            //模式控制写入字节       
#define TIMER_FREQ      1193182L        //输入频率
#define HZ              1000            //时钟中断频率为1000hz



void	timecounter_inc();
size_t	clock();




#endif