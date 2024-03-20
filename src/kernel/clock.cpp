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
bool volatile beeping = false;


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

void clockHandler(int vector)
{
    assert(vector == 0x20, "The clock interrupt number is incorrect.");
    sendEoi(vector); // 发送中断处理结束
    stopBeep();
    jiffies++;
    // DEBUGK("clock jiffies %d ...\n", jiffies);
    TASK_INFO *task = runningTask();
    // printk("magic: %p address: %p", task->magic, task);
    assert(task->magic == SOUL_MAGIC, "Task page stack overflow causes task PCB information to be damaged.");

    task->jiffies = jiffies;
    task->ticks--;
    if (!task->ticks)
    {
        task->ticks = task->priority;
        schedule();
    }
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
    setInterruptHandler(IRQ_CLOCK, (void*)clockHandler);
    setInterruptMask(IRQ_CLOCK, true);
}