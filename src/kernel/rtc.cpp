#include <kernel/rtc.h>
#include <io/io.h>
#include <kernel/time.h>
#include <lib/stdlib.h>
#include <kernel/interrupt.h>
#include <kernel/assert.h>
#include <kernel/printk.h>

#define CMOS_ADDR 0x70 // CMOS 地址寄存器
#define CMOS_DATA 0x71 // CMOS 数据寄存器

#define CMOS_SECOND 0x01
#define CMOS_MINUTE 0x03
#define CMOS_HOUR 0x05

#define CMOS_A 0x0a
#define CMOS_B 0x0b
#define CMOS_C 0x0c
#define CMOS_D 0x0d
#define CMOS_NMI 0x80

void setAlarm(uint32 secs)
{
    TM_INFO time;
    timeRead(&time);

    uint8 sec = secs % 60;
    secs /= 60;
    uint8 min = secs % 60;
    secs /= 60;
    uint32 hour = secs;

    time.sec += sec;
    if (time.sec >= 60)
    {
        time.sec %= 60;
        time.min += 1;
    }

    time.min += min;
    if (time.min >= 60)
    {
        time.min %= 60;
        time.hour += 1;
    }

    time.hour += hour;
    if (time.hour >= 24)
    {
        time.hour %= 24;
    }

    cmosWrite(CMOS_HOUR, binToBcd(time.hour));
    cmosWrite(CMOS_MINUTE, binToBcd(time.min));
    cmosWrite(CMOS_SECOND, binToBcd(time.sec));

    cmosWrite(CMOS_B, 0b00100010); // 打开闹钟中断
    cmosRead(CMOS_C);              // 读 C 寄存器，以允许 CMOS 中断
}

uint8 cmosRead(uint8 addr)
{
    outByte(CMOS_ADDR, CMOS_NMI | addr);
    return inByte(CMOS_DATA);
}

void cmosWrite(uint8 addr, uint8 value)
{
    outByte(CMOS_ADDR, CMOS_NMI | addr);
    outByte(CMOS_DATA, value);
}


extern void startBeep();

uint32 counter = 0;
// 实时时钟中断处理函数
void rtcHandler(int32 vector)
{
    // 实时时钟中断向量号
    assert(vector == 0x28, "Real-time clock interrupt vector number error");

    // 向中断控制器发送中断处理完成的信号
    sendEoi(vector);

    // 读 CMOS 寄存器 C，允许 CMOS 继续产生中断
    // cmosRead(CMOS_C);
    // setAlarm(5);
    // startBeep();
    printk("RTC handler %d .\n", counter++);
}

void rtcInit()
{
    // cmosWrite(CMOS_B, 0b01000010); // 打开周期中断
    // cmosWrite(CMOS_B, 0b001000010); // 打开闹钟中断
    // cmosRead(CMOS_C); // 读 C 寄存器，以便允许 CMOS 中断
    // setAlarm(5);
    // 设置中断频率
    // outByte(CMOS_A, (inByte(CMOS_A) & 0xf) | 0b1110);

    setInterruptHandler(IRQ_RTC, rtcHandler);
    setInterruptMask(IRQ_RTC, true);
    setInterruptMask(IRQ_CASCADE, true);
}