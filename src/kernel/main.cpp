#include <kernel/kernel.h>
#include <io/cursor.h>
#include <lib/charArray.h>
#include <kernel/console.h>
#include <kernel/printk.h>
#include <kernel/debug.h>

const unsigned int magic = SOUL_MAGIC;
char message[1024] = "Hello SoulOS C++...";
char buf[1024];

extern "C" void kernelInit()
{
    consoleInit();
    for(int i = 0;i < 100; i++)
    {
        printk("[TEST context] : %s (%d)\n", message, i+1);
    }
    BREAK_POINT;
    DEBUGK("debug soulos!!!\n");
}

// 输入输出 √
// 字符串处理 √
// 显卡驱动 √
// 可变参数 √
// printk √
// 断言 √
// 调试 √
// 内核的全局描述符表
// 任务及上下文
// 中断函数
// 中断描述符
// 异常处理
// 外中断
// 中断上下文
// 计数器和时钟
// 蜂鸣器
// 时间
// 实时时钟
// 内存管理
// 物理内存管理
// 内核内存映射
// 数据结构位图
// 内核虚拟内存管理
// 外中断控制
// 创建内核线程
// === === ===
// 系统调用
// 系统调用 yield
// 数据结构链表
// 任务阻塞和就绪
// 基础任务
// 任务睡眠和任务唤醒
// 互斥和信号量
// 锁
// multiboot
// 互斥锁
// 键盘中断
// 键盘驱动
// 数据结构循环队列
// 用户模式
// 用户模式printf
// 内核堆内存管理
// 用户内存映射
// 进程用户态栈
// 系统调用 brk
// 任务 ID
// 系统调用 fork
// 系统调用 exit
// 系统调用 waitpid
// 系统调用 time
// 硬盘同步 PIO
// 硬盘异步 PIO
// 识别硬盘
// 硬盘分区
// 虚拟设备
// 块设备请求
// 磁盘调度电梯算法
// 哈希表和高速缓冲
// 创建文件系统
// 根超级块
// 文件系统位图操作
// 文件系统 inode
// 文件系统状态
// 文件系统目录操作
// 文件系统 namei
// 读写 inode
// 截断 inode
// 系统调用 mkdir,rmdir,link,unlink
// 打开 inode
// 文件初始化
// 系统调用 open,close,read,write,lseek,getcwd,chdir,osh,stat,fstat,mknod,mount,umount
// 格式化文件系统
// 虚拟磁盘
// 标准输入输出
// 系统调用 mmap, munmap
// ELF 文件解析和符号解析
// 程序加载和执行
// 串口设备驱动
// 串口输出日志
// C 运行时环境
// 参数和环境变量 & 基础命令
// 输入输出重定向
// 管道 & 管道序列
// init 进程
// 内核内存保护 & 内核内存保护配置
// CSI 控制序列
// 错误处理机制 - 堆内存页缓存
// 定时器
// 任务会话
// TTY 设备
// 信号
// 闹钟
// 异常与调试
// CPU 功能检测
// FPU 浮点运算单元 & 基础浮点运算
//  ISA 总线
// 声卡驱动
// 软盘驱动
// PCI 总线
// 系统优化 - 映射物理内存 & 内核扩容 & 兼容 bochs 串口 & 文件重命名
// 磁盘驱动优化 - 磁盘类型检测 & 磁盘错误处理
// IDE 硬盘 UDMA
// 通信
// e1000 网卡驱动
// 数据包高速缓冲
// CRC 校验和
// 虚拟网络设备
// 以太网协议实现 & ARP 协议 & ARP 缓存
// IP 协议 & ICMP 协议 & IP 校验和
// 虚拟文件系统
// 套接字 & 数据包套接字 & 原始套接字超时设置
// ping
// localhost 环回地址
// UDP & TCP
// TCP 定时器 & TCP 断开连接 & TCP 服务端连接测试 & TCP 客户端连接测试
// TCP 窗口管理 & Nagle 算法 & 超时重传 & 拥塞控制
// DHCP 协议
// DNS 域名解析
// 高速缓冲优化 & 内核校验和
// ATAPI 光盘驱动
// ISO9660 文件系统

// === === ==
// 论文
