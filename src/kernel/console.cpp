#include <kernel/console.h>
#include <io/cursor.h>
#include <lib/charArray.h>

static ConsoleData consoleData;

// 设置光标位置
static inline void setXY(ConsoleData *_con, uint32 _x, uint32 _y)
{
    if (_x > _con->width || _y >= _con->height) return;
    _con->x = _x;
    _con->y = _y;
    _con->pos = _con->screen + _y * _con->row_size + (_x << 1);
}

// 保存光标位置
static inline void saveCursor(ConsoleData *_con)
{
    _con->saved_x = _con->x;
    _con->saved_y = _con->y;
}

// 清除屏幕
static inline void eraseScreen(ConsoleData *_con, uint16 *_start, uint32 _count)
{
    int nr = 0;
    while (nr++ < _count)
    {
        *_start++ = _con->erase;
    }
}

// 清空控制台
void consoleClear(ConsoleData *_con)
{
    _con->screen = _con->mem_base;
    _con->pos = _con->mem_base;
    _con->x = _con->y = 0;

    // 有移动一位是因为显示的内存地址 2byte 表示一个字符，所以字符的位置要对内存的长度除以二
    setCursor((uint16)((_con->pos - _con->mem_base) >> 1));
    setScreen((uint16)((_con->screen - _con->mem_base) >> 1));
    eraseScreen(_con, (uint16 *)_con->mem_base, _con->mem_size >> 1);
}

// 向上滚屏
static void scrollUp(ConsoleData *_con)
{
    if ((_con->screen + _con->scr_size + _con->row_size) >= _con->mem_end)
    {
        memcpy((void *)_con->mem_base, (void *)_con->screen, _con->scr_size);
        _con->pos -= (_con->screen - _con->mem_base);
        _con->screen = _con->mem_base;
    }

    uint16 *ptr = (uint16 *)(_con->screen + _con->scr_size);
    eraseScreen(_con, ptr, _con->width);

    _con->screen += _con->row_size;
    _con->pos += _con->row_size;
    setScreen((uint16)((_con->screen - _con->mem_base) >> 1));
}

// 向下滚屏
static void scrollDown(ConsoleData *_con)
{
    _con->screen -= _con->row_size;
    if (_con->screen < _con->mem_base)
    {
        _con->screen = _con->mem_base;
    }
    setScreen((uint16)((_con->screen - _con->mem_base) >> 1));
}

// 换行 \n
static inline void lf(ConsoleData *_con)
{
    if (_con->y + 1 < _con->height)
    {
        _con->y++;
        _con->pos += _con->row_size;
        return;
    }
    scrollUp(_con);
}

// 光标回到开始位置
static inline void cr(ConsoleData *_con)
{
    _con->pos -= (_con->x << 1);
    _con->x = 0;
}

// TAB
static inline void tab(ConsoleData *_con)
{
    int offset = 8 - (_con->x & 7);
    _con->x += offset;
    _con->pos += offset << 1;
    if (_con->x >= _con->width)
    {
        _con->x -= _con->width;
        _con->pos -= _con->row_size;
        lf(_con);
    }
}

// 退格
static inline void bs(ConsoleData *_con)
{
    if (!_con->x)
        return;
    _con->x--;
    _con->pos -= 2;
    *(uint16 *)_con->pos = _con->erase;
}

// 删除当前字符
static inline void del(ConsoleData *_con)
{
    *(uint16 *)_con->pos = _con->erase;
}

// 输出字符
static inline void chr(ConsoleData *_con, char ch)
{
    if (_con->x >= _con->width)
    {
        _con->x -= _con->width;
        _con->pos -= _con->row_size;
        lf(_con);
    }

    *_con->ptr++ = ch;
    *_con->ptr++ = _con->style;
    _con->x++;
}

// extern void start_beep();

// 正常状态
static inline void stateNormal(ConsoleData *_con, char ch)
{
    switch (ch)
    {
    case NUL:
        break;
    case BEL:
        // start_beep();
        break;
    case BS:
        bs(_con);
        break;
    case HT:
        tab(_con);
        break;
    case LF:
        lf(_con);
        cr(_con);
        break;
    case VT:
    case FF:
        lf(_con);
        break;
    case CR:
        cr(_con);
        break;
    case DEL:
        del(_con);
        break;
    case ESC:
        _con->state = STATE_ESC;
        break;
    default:
        chr(_con, ch);
        break;
    }
}

// esc 状态
static inline void stateEsc(ConsoleData *_con, char ch)
{
    switch (ch)
    {
    case '[':
        _con->state = STATE_QUE;
        break;
    case 'E':
        lf(_con);
        cr(_con);
        break;
    case 'M':
        // go up
        break;
    case 'D':
        lf(_con);
        break;
    case 'Z':
        // response
        break;
    case '7':
        saveCursor(_con);
        break;
    case '8':
        setXY(_con, _con->saved_x, _con->saved_y);
        break;
    default:
        break;
    }
}

// 参数状态
static inline bool stateArg(ConsoleData *_con, char ch)
{
    if (_con->argc >= ARG_NR)
        return false;
    if (ch == ';')
    {
        _con->argc++;
        return false;
    }
    if (ch >= '0' && ch <= '9')
    {
        _con->args[_con->argc] = _con->args[_con->argc] * 10 + ch - '0';
        return false;
    }
    _con->argc++;
    _con->state = STATE_CSI;
    return true;
}

// 清屏
static inline void csiJ(ConsoleData *_con)
{
    int count = 0;
    int start = 0;
    switch (_con->args[0])
    {
    case 0: // 擦除屏幕中光标后面的部分
        count = (_con->screen + _con->scr_size - _con->pos) >> 1;
        start = _con->pos;
        break;
    case 1: // 擦除屏幕中光标前面的部分
        count = (_con->pos - _con->screen) >> 1;
        start = _con->screen;
        break;
    case 2: // 整个屏幕上的字符
        count = _con->scr_size >> 1;
        start = _con->screen;
        break;
    default:
        return;
    }

    eraseScreen(_con, (uint16 *)start, count);
}

// 删除行
static inline void csiK(ConsoleData *_con)
{
    int count = 0;
    int start = 0;
    switch (_con->args[0])
    {
    case 0: // 删除行光标后
        count = _con->width - _con->x;
        start = _con->pos;
        break;
    case 1: // 删除行光标前
        count = _con->x;
        start = _con->pos - (_con->x << 1);
        break;
    case 2: // 删除整行
        count = _con->width;
        start = _con->pos - (_con->x << 1);
        break;
    default:
        return;
    }

    eraseScreen(_con, (uint16 *)start, count);
}

// 插入一行
static inline void insertLine(ConsoleData *_con)
{
    uint16 *start = (uint16 *)(_con->screen + _con->y * _con->row_size);
    for (size_t i = 2; true; i++)
    {
        void *src = (void *)(_con->mem_end - (i * _con->row_size));
        if (src < (void *)start)
            break;

        memcpy((void*)((uint32)src + _con->row_size), src, (size_t)_con->row_size);
    }
    eraseScreen(_con, (uint16 *)(_con->screen + (_con->y) * _con->row_size), _con->width);
}

// 插入多行
static inline void csiL(ConsoleData *_con)
{
    int nr = _con->args[0];
    if (nr > _con->height)
        nr = _con->height;
    else if (!nr)
        nr = 1;
    while (nr--)
    {
        insertLine(_con);
    }
}

// 删除一行
static inline void deleteLine(ConsoleData *_con)
{
    uint16 *start = (uint16 *)(_con->screen + _con->y * _con->row_size);
    for (size_t i = 1; true; i++)
    {
        void *src = start + (i * _con->row_size);
        if (src >= (void *)_con->mem_end)
            break;

        memcpy((void *)((uint32)src - _con->row_size), src, (size_t)_con->row_size);
    }
    eraseScreen(_con, (uint16 *)(_con->mem_end - _con->row_size), _con->width);
}

// 删除多行
static inline void csiM(ConsoleData *_con)
{
    int nr = _con->args[0];
    if (nr > _con->height)
        nr = _con->height;
    else if (!nr)
        nr = 1;
    while (nr--)
    {
        deleteLine(_con);
    }
}

// 删除当前字符
static inline void deleteChar(ConsoleData *_con)
{
    uint16 *ptr = (uint16 *)_con->ptr;
    uint16 i = _con->x;
    while (++i < _con->width)
    {
        *ptr = *(ptr + 1);
        ptr++;
    }
    *ptr = _con->erase;
}

// 删除多个字符
static inline void csiP(ConsoleData *_con)
{
    int nr = _con->args[0];
    if (nr > _con->height)
        nr = _con->height;
    else if (!nr)
        nr = 1;
    while (nr--)
    {
        deleteChar(_con);
    }
}

// 插入字符
static inline void insertChar(ConsoleData *_con)
{
    uint16 *ptr = (uint16 *)_con->ptr + (_con->width - _con->x - 1);
    while (ptr > (uint16 *)_con->ptr)
    {
        *ptr = *(ptr - 1);
        ptr--;
    }
    *_con->ptr = _con->erase;
}

// 插入多个字符
static inline void csiAt(ConsoleData *_con)
{
    int nr = _con->args[0];
    if (nr > _con->height)
        nr = _con->height;
    else if (!nr)
        nr = 1;
    while (nr--)
    {
        insertChar(_con);
    }
}

// 修改样式
static inline void csi_m(ConsoleData *_con)
{
    _con->style = 0;
    for (size_t i = 0; i < _con->argc; i++)
    {
        if (_con->args[i] == ST_NORMAL)
            _con->style = STYLE;

        else if (_con->args[i] == ST_BOLD)
            _con->style = BOLD;

        else if (_con->args[i] == BLINK)
            _con->style |= BLINK;

        else if (_con->args[i] == ST_REVERSE)
            _con->style = (_con->style >> 4) | (_con->style << 4);

        else if (_con->args[i] >= 30 && _con->args[i] <= 37)
            _con->style = _con->style & 0xF8 | (_con->args[i] - 30);

        else if (_con->args[i] >= 40 && _con->args[i] <= 47)
            _con->style = _con->style & 0x8F | ((_con->args[i] - 40) << 4);
    }
    _con->erase = (_con->style << 8) | 0x20;
}

// CSI 状态
static inline void stateCsi(ConsoleData *_con, char ch)
{
    _con->state = STATE_NOR;
    switch (ch)
    {
    case 'G':
    case '`':
        if (_con->args[0])
            _con->args[0]--;
        setXY(_con, _con->args[0], _con->y);
        break;
    case 'A': // 光标上移一行或 n 行
        if (!_con->args[0])
            _con->args[0]++;
        setXY(_con, _con->x, _con->y - _con->args[0]);
        break;
    case 'B':
    case 'e': // 光标下移一行或 n 行
        if (!_con->args[0])
            _con->args[0]++;
        setXY(_con, _con->x, _con->y + _con->args[0]);
        break;
    case 'C':
    case 'a': // 光标右移一列或 n 列
        if (!_con->args[0])
            _con->args[0]++;
        setXY(_con, _con->x + _con->args[0], _con->y);
        break;
    case 'D': // 光标左移一列或 n 列
        if (!_con->args[0])
            _con->args[0]++;
        setXY(_con, _con->x - _con->args[0], _con->y);
        break;
    case 'E': // 光标下移一行或 n 行，并回到 0 列
        if (!_con->args[0])
            _con->args[0]++;
        setXY(_con, 0, _con->y + _con->args[0]);
        break;
    case 'F': // 光标上移一行或 n 行，并回到 0 列
        if (!_con->args[0])
            _con->args[0]++;
        setXY(_con, 0, _con->y - _con->args[0]);
        break;
    case 'd': // 设置行号
        if (_con->args[0])
            _con->args[0]--;
        setXY(_con, _con->x, _con->args[0]);
        break;
    case 'H': // 设置行号和列号
    case 'f':
        if (_con->args[0])
            _con->args[0]--;
        if (_con->args[1])
            _con->args[1]--;
        setXY(_con, _con->args[1], _con->args[0]);
        break;
    case 'J': // 清屏
        csiJ(_con);
        break;
    case 'K': // 行删除
        csiK(_con);
        break;
    case 'L': // 插入行
        csiL(_con);
        break;
    case 'M': // 删除行
        csiM(_con);
        break;
    case 'P': // 删除字符
        csiP(_con);
        break;
    case '@': // 插入字符
        csiAt(_con);
        break;
    case 'm': // 修改样式
        csi_m(_con);
        break;
    case 'r': // 设置起始行号和终止行号
        break;
    case 's':
        saveCursor(_con);
    case 'u':
        setXY(_con, _con->saved_x, _con->saved_y);
    default:
        break;
    }
}

// 写控制台
int consoleWrite(ConsoleData *_con, char *buf, uint32 count)
{
    char ch;
    int nr = 0;
    while (nr++ < count)
    {
        ch = *buf++;

        switch (_con->state)
        {
        case STATE_NOR:
            stateNormal(_con, ch);
            break;
        case STATE_ESC:
            stateEsc(_con, ch);
            break;
        case STATE_QUE:
            memset(_con->args, 0, sizeof(_con->args));
            _con->argc = 0;
            _con->state = STATE_ARG;
            _con->ques = (ch == '?');
            if (_con->ques)
                break;
        case STATE_ARG:
            if (!stateArg(_con, ch))
                break;
        case STATE_CSI:
            stateCsi(_con, ch);
            break;
        default:
            break;
        }
    }
    setCursor((uint16)((_con->pos - _con->mem_base) >> 1));
    // 恢复中断
    return nr;
}

void consoleInit()
{
    ConsoleData *_con = &consoleData;
    _con->mem_base = MEM_BASE;
    _con->mem_size = (MEM_SIZE / ROW_SIZE) * ROW_SIZE;
    _con->mem_end = _con->mem_base + _con->mem_size;
    _con->width = WIDTH;
    _con->height = HEIGHT;
    _con->row_size = _con->width * 2;
    _con->scr_size = _con->width * _con->height * 2;

    _con->erase = ERASE;
    _con->style = STYLE;
    consoleClear(_con);
}

int testConsoleWrite(char *buf, uint32 count)
{
    return consoleWrite(&consoleData, buf, count);
}