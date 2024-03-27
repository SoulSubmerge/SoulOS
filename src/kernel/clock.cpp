#include <kernel/clock.h>
#include <io/io.h>
#include <types/types.h>
#include <kernel/assert.h>
#include <kernel/interrupt.h>
#include <kernel/debug.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/printk.h>

uint32 volatile jiffies = 0;
uint32 jiffy = JIFFY;
uint32 volatile beeping = 0;


void startBeep()
{
    if (!beeping)
    {
        outByte(SPEAKER_REG, inByte(SPEAKER_REG) | 0b11);
        // beeping = true;
        // // taskSleep(BEEP_MS); TODO: 需要实现
        // outByte(SPEAKER_REG, inByte(SPEAKER_REG) & 0xfc);
        // beeping = false;
    }
    beeping = jiffies + 5;
}

void stopBeep()
{
    if (beeping && jiffies > beeping)
    {
        outByte(SPEAKER_REG, inByte(SPEAKER_REG) & 0xfc);
        beeping = 0;
    }
}

extern void taskWakeup();

void clockHandler(int vector)
{
    assert(vector == 0x20, "The clock interrupt number is incorrect.");
    sendEoi(vector); // 发送中断处理结束
    stopBeep();
    taskWakeup();

    jiffies++;
    // DEBUGK("clock jiffies %d ...\n", jiffies);
    task_t *task = runningTask();
    assert(task->magic == SOUL_MAGIC, "Task page stack overflow causes task PCB information to be damaged.");

    task->jiffies = jiffies;
    task->ticks--;
    if (!task->ticks)
    {
        schedule();
    }
}

extern uint32 startupTime;

time_t sysTime()
{
    return startupTime + (jiffies * JIFFY) / 1000;
}

void pitInit()
{
    // 配置计数器 0 时钟
    outByte(PIT_CTRL_REG, 0b00110100);
    outByte(PIT_CHAN0_REG, CLOCK_COUNTER & 0xff);
    outByte(PIT_CHAN0_REG, (CLOCK_COUNTER >> 8) & 0xff);

    // 配置计数器 2 蜂鸣器
    outByte(PIT_CTRL_REG, 0b10110110);
    outByte(PIT_CHAN2_REG, (uint8)BEEP_COUNTER);
    outByte(PIT_CHAN2_REG, (uint8)(BEEP_COUNTER >> 8));
}

void clockInit()
{
    pitInit();
    setInterruptHandler(IRQ_CLOCK, clockHandler);
    setInterruptMask(IRQ_CLOCK, true);
}