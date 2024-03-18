#include <lib/stdlib.h>

// 延迟
void delay(uint64 _count)
{
    while (_count--);
}

// 阻塞函数
void hang()
{
    while (true);
}

char toupper(char _ch)
{
    if (_ch >= 'a' && _ch <= 'z')
        _ch -= 0x20;
    return _ch;
}

char tolower(char _ch)
{
    if (_ch >= 'A' && _ch <= 'Z')
        _ch += 0x20;
    return _ch;
}

// 将 bcd 码转成整数
uint8 bcdToBin(uint8 _value)
{
    return (_value & 0xf) + (_value >> 4) * 10;
}

// 将整数转成 bcd 码
uint8 binToBcd(uint8 _value)
{
    return (_value / 10) * 0x10 + (_value % 10);
}

// 计算 num 分成 size 的数量
uint32 divRoundUp(uint32 _num, uint32 _size)
{
    return (_num + _size - 1) / _size;
}

// 判断是否是数字
bool isdigit(int32 _c)
{
    return _c >= '0' && _c <= '9';
}

int32 atoi(const char *_str)
{
    if (_str == nullptr)
        return 0;
    int sign = 1;
    int result = 0;
    if (*_str == '-')
    {
        sign = -1;
        _str++;
    }
    for (; *_str; _str++)
    {
        result = result * 10 + (*_str - '0');
    }
    return result * sign;
}