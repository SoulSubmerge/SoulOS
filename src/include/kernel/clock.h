#ifndef SOUL_CLOCK_H
#define SOUL_CLOCK_H


#define HZ 100 // 基数
#define OSCILLATOR 1193182 // 振荡器的频率
#define CLOCK_COUNTER (OSCILLATOR / HZ) // 时钟发生中断的计数次数，但减少到0则发生一次时钟中断
#define JIFFY (1000 / HZ)

#define BEEP_HZ 440
#define BEEP_COUNTER (OSCILLATOR / BEEP_HZ)
#define BEEP_MS 100

#endif