#ifndef MINIOS_TIME_H
#define MINIOS_TIME_H

#include "type.h"
#define TIMER0          0X40            //counter0�˿�
#define TIMER_MODE      0x43            //ģʽ���ƼĴ����˿�
#define RATE_GENERATOR  0X34            //ģʽ����д���ֽ�       
#define TIMER_FREQ      1193182L        //����Ƶ��
#define HZ              1000            //ʱ���ж�Ƶ��Ϊ1000hz



void	timecounter_inc();
size_t	clock();




#endif