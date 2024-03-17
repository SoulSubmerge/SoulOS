#include <io/cursor.h>
#include <io/io.h>

void setCursor(uint16 _value)
{
    outByte(CRT_ADDR_REG, CRT_CURSOR_H);
    outByte(CRT_DATA_REG, (uint8)((_value >> 8) & 0xff));
    outByte(CRT_ADDR_REG, CRT_CURSOR_L);
    outByte(CRT_DATA_REG, (uint8)(_value & 0xff));
}

uint16 getCursor()
{
    outByte(CRT_ADDR_REG, CRT_CURSOR_H);
    uint16 pos = inByte(CRT_DATA_REG);
    pos = pos << 8;
    outByte(CRT_ADDR_REG, CRT_CURSOR_L);
    pos |= (inByte(CRT_DATA_REG) | 0x00ff);
    return pos;
}

void setScreen(uint16 _value)
{
    outByte(CRT_ADDR_REG, CRT_START_ADDR_H);
    outByte(CRT_DATA_REG, (uint8)((_value >> 8) & 0xff));
    outByte(CRT_ADDR_REG, CRT_START_ADDR_L);
    outByte(CRT_DATA_REG, (uint8)(_value & 0xff));
}