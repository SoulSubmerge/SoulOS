#include <kernel/time.h>
#include <lib/stdlib.h>
#include <kernel/rtc.h>
#include <kernel/printk.h>

// 下面是 CMOS 信息的寄存器索引
#define CMOS_SECOND 0x00  // (0 ~ 59)
#define CMOS_MINUTE 0x02  // (0 ~ 59)
#define CMOS_HOUR 0x04    // (0 ~ 23)
#define CMOS_WEEKDAY 0x06 // (1 ~ 7) 星期天 = 1，星期六 = 7
#define CMOS_DAY 0x07     // (1 ~ 31)
#define CMOS_MONTH 0x08   // (1 ~ 12)
#define CMOS_YEAR 0x09    // (0 ~ 99)
#define CMOS_CENTURY 0x32 // 可能不存在
#define CMOS_NMI 0x80

#define MINUTE 60          // 每分钟的秒数
#define HOUR (60 * MINUTE) // 每小时的秒数
#define DAY (24 * HOUR)    // 每天的秒数
#define YEAR (365 * DAY)   // 每年的秒数，以 365 天算

// 每个月开始时的已经过去天数
static uint16 month[13] = {
    0, // 这里占位，没有 0 月，从 1 月开始
    0,
    (31),
    (31 + 29),
    (31 + 29 + 31),
    (31 + 29 + 31 + 30),
    (31 + 29 + 31 + 30 + 31),
    (31 + 29 + 31 + 30 + 31 + 30),
    (31 + 29 + 31 + 30 + 31 + 30 + 31),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)};


time_t startupTime;
int32 century;

int32 elapsedLeapYears(int32 _year)
{
    int32 result = 0;
    result += (_year - 1) / 4;
    result -= (_year - 1) / 100;
    result += (_year + 299) / 400;
    result -= (1970 - 1900) / 4;
    return result;
}

bool isLeapYear(int32 _year)
{
    return ((_year % 4 == 0) && (_year % 100 != 0)) || ((_year + 1900) % 400 == 0);
}

void localtime(time_t _stamp, tm *_time)
{
    _time->sec = _stamp % 60;

    time_t remain = _stamp / 60;

    _time->min = remain % 60;
    remain /= 60;

    _time->hour = remain % 24;
    time_t days = remain / 24;

    _time->wday = (days + 4) % 7; // 1970-01-01 是星期四

    // 这里产生误差显然需要 365 个闰年，不管了
    int32 years = days / 365 + 70;
    _time->year = years;
    int32 offset = 1;
    if (isLeapYear(years))
        offset = 0;

    days -= elapsedLeapYears(years);
    _time->yday = days % (366 - offset);

    int32 mon = 1;
    for (; mon < 13; mon++)
    {
        if ((month[mon] - offset) > _time->yday)
            break;
    }

    _time->mon = mon - 1;
    _time->mday = _time->yday - month[_time->mon] + offset + 1;
}

int32 getYday(tm *_time)
{
    int32 res = month[_time->mon]; // 已经过去的月的天数
    res += _time->mday;          // 这个月过去的天数

    int32 year;
    if (_time->year >= 70)
        year = _time->year - 70;
    else
        year = _time->year - 70 + 100;

    // 如果不是闰年，并且 2 月已经过去了，则减去一天
    // 注：1972 年是闰年，这样算不太精确，忽略了 100 年的平年
    if ((year + 2) % 4 && _time->mon > 2)
    {
        res -= 1;
    }

    return res;
}

void timeReadBcd(tm *_time)
{
    // CMOS 的访问速度很慢。为了减小时间误差，在读取了下面循环中所有数值后，
    // 若此时 CMOS 中秒值发生了变化，那么就重新读取所有值。
    // 这样内核就能把与 CMOS 的时间误差控制在 1 秒之内。
    do
    {
        _time->sec = cmosRead(CMOS_SECOND);
        _time->min = cmosRead(CMOS_MINUTE);
        _time->hour = cmosRead(CMOS_HOUR);
        _time->wday = cmosRead(CMOS_WEEKDAY);
        _time->mday = cmosRead(CMOS_DAY);
        _time->mon = cmosRead(CMOS_MONTH);
        _time->year = cmosRead(CMOS_YEAR);
        century = cmosRead(CMOS_CENTURY);
    } while (_time->sec != cmosRead(CMOS_SECOND));
}


void timeRead(tm *_time)
{
    timeReadBcd(_time);
    _time->sec = bcdToBin(_time->sec);
    _time->min = bcdToBin(_time->min);
    _time->hour = bcdToBin(_time->hour);
    _time->wday = bcdToBin(_time->wday);
    _time->mday = bcdToBin(_time->mday);
    _time->mon = bcdToBin(_time->mon);
    _time->year = bcdToBin(_time->year);
    _time->yday = getYday(_time);
    _time->isdst = -1;
    century = bcdToBin(century);
}

time_t mktime(tm *_time)
{
    time_t res;
    int year; // 1970 年开始的年数
    // 下面从 1900 年开始的年数计算
    if (_time->year >= 70)
        year = _time->year - 70;
    else
        year = _time->year - 70 + 100;

    // 这些年经过的秒数时间
    res = YEAR * year;

    // 已经过去的闰年，每个加 1 天
    res += DAY * ((year + 1) / 4);

    // 已经过完的月份的时间
    res += month[_time->mon] * DAY;

    // 如果 2 月已经过了，并且当前不是闰年，那么减去一天
    if (_time->mon > 2 && ((year + 2) % 4))
        res -= DAY;

    // 这个月已经过去的天
    res += DAY * (_time->mday - 1);

    // 今天过去的小时
    res += HOUR * _time->hour;

    // 这个小时过去的分钟
    res += MINUTE * _time->min;

    // 这个分钟过去的秒
    res += _time->sec;

    return res;
}

void timeInit()
{
    tm time;
    timeRead(&time);
    startupTime = mktime(&time);
    printk("startup time: %d%d-%02d-%02d %02d:%02d:%02d\n",
         century,
         time.year,
         time.mon,
         time.mday,
         time.hour,
         time.min,
         time.sec);
}