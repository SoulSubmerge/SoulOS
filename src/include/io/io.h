#ifndef SOUL_IO_H
#define SOUL_IO_H

#include <types/types.h>


// - CRT 地址寄存器 0x3D4
// - CRT 数据寄存器 0x3D5
// - CRT 光标位置 - 高位 0xE
// - CRT 光标位置 - 低位 0xF

#define CRT_ADDR_REG 0x3d4 // CRT(6845)索引寄存器
#define CRT_DATA_REG 0x3d5 // CRT(6845)数据寄存器

#define CRT_CURSOR_H 0xe // 光标位置 - 高位
#define CRT_CURSOR_L 0xf // 光标位置 - 低位
#define CRT_START_ADDR_H 0xc // 显示内存起始位置 - 高位
#define CRT_START_ADDR_L 0xd // 显示内存起始位置 - 低位

// 中断相关的
#define PIC_M_CTRL 0x20 // 主片的控制端口
#define PIC_M_DATA 0x21 // 主片的数据端口
#define PIC_S_CTRL 0xa0 // 从片的控制端口
#define PIC_S_DATA 0xa1 // 从片的数据端口
#define PIC_EOI 0x20    // 通知中断控制器中断结束

extern "C" uint16 inWord(uint16 _port);
extern "C" void outWord(uint16 _port, uint16 _value);


extern "C" uint8 inByte(uint16 _port);
extern "C" void outByte(uint16 _port, uint8 _value);


#endif