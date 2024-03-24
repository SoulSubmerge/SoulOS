#ifndef SOUL_CONSOLE_H
#define SOUL_CONSOLE_H
#include <types/types.h>

#define ARG_NR 16
#define ERASE 0x0720

#define MEM_BASE 0xB8000              // 显卡内存起始位置
#define MEM_SIZE 0x4000               // 显卡内存大小
#define MEM_END (MEM_BASE + MEM_SIZE) // 显卡内存结束位置
#define WIDTH 80                      // 屏幕文本列数
#define HEIGHT 25                     // 屏幕文本行数
#define ROW_SIZE (WIDTH * 2)          // 每行字节数
#define SCR_SIZE (ROW_SIZE * HEIGHT)  // 屏幕字节数

#define NUL 0x00
#define ENQ 0x05
#define ESC 0x1B // ESC
#define BEL 0x07 // \a
#define BS 0x08  // \b
#define HT 0x09  // \t
#define LF 0x0A  // \n
#define VT 0x0B  // \v
#define FF 0x0C  // \f
#define CR 0x0D  // \r
#define DEL 0x7F

enum ST
{
    ST_NORMAL = 0,
    ST_BOLD = 1,
    ST_BLINK = 5,
    ST_REVERSE = 7,
};

#define STYLE 7
#define BLINK 0x80
#define BOLD 0x0F
#define UNDER 0X0F

enum COLOR
{
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    YELLOW = 6,
    WHITE = 7,
};

enum state
{
    STATE_NOR,
    STATE_ESC,
    STATE_QUE,
    STATE_ARG,
    STATE_CSI,
};

class ConsoleData
{
public:
    uint32 mem_base; // 内存基地址
    uint32 mem_size; // 内存大小
    uint32 mem_end;  // 内存结束位置

    uint32 screen;   // 当前屏幕位置
    uint32 scr_size; // 屏幕内存大小

    union
    {
        uint32 pos;   // 当前光标位置
        char *ptr; // 位置指针
    };
    uint32 x;        // 光标坐标 x
    uint32 y;        // 光标坐标 y
    uint32 saved_x;  // 保存的 x
    uint32 saved_y;  // 保存的 y
    uint32 width;    // 屏幕宽度
    uint32 height;   // 屏幕高度
    uint32 row_size; // 行内存大小

    uint8 state;         // 当前状态
    uint32 args[ARG_NR]; // 参数
    uint32 argc;         // 参数数量
    uint32 ques;         //

    uint16 erase; // 清屏字符
    uint8 style;  // 当前样式
};

int consoleWrite(char *buf, uint32 count); // 占时用于调用显卡写入字符的
// void consoleClear();




#endif