#ifndef SOUL_TIME_H
#define SOUL_TIME_H

#include <types/types.h>


typedef struct tm
{
    int32 sec;   // 秒数 [0，59]
    int32 min;   // 分钟数 [0，59]
    int32 hour;  // 小时数 [0，59]
    int32 mday;  // 1 个月的天数 [0，31]
    int32 mon;   // 1 年中月份 [0，11]
    int32 year;  // 从 1900 年开始的年数
    int32 wday;  // 1 星期中的某天 [0，6] (星期天 =0)
    int32 yday;  // 1 年中的某天 [0，365]
    int32 isdst; // 夏令时标志
}tm;

void timeReadBcd(tm *_time);
void timeRead(tm *_time);
time_t mktime(tm *_time);
void localtime(time_t _stamp, tm *_time);

#endif