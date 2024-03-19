#ifndef SOUL_RTC_H
#define SOUL_RTC_H

#include <types/types.h>

void setAlarm(uint32 secs);
uint8 cmosRead(uint8 addr);
void cmosWrite(uint8 addr, uint8 value);

#endif