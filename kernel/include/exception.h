#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdbool.h>

#include "console.h"

/* CSR 寄存器地址 */
#define CSR_CRMD 0x0   /* 当前模式寄存器 */
#define CSR_ECFG 0x4   /* 异常配置寄存器 */
#define CSR_ESTAT 0x5  /* 异常状态寄存器 */
#define CSR_EENTRY 0xc /* 异常入口寄存器 */
#define CSR_TCFG 0x41  /* 定时器配置寄存器 */
#define CSR_TICLR 0x44 /* 定时器中断清除寄存器 */

/* CSR 寄存器位定义 */
#define CSR_CRMD_IE (1UL << 2)       /* 全局中断使能位 */
#define CSR_TCFG_EN (1UL << 0)       /* 定时器启用位 */
#define CSR_TCFG_PER (1UL << 1)      /* 定时器周期模式位 */
#define CSR_TICLR_CLR (1UL << 0)     /* 定时器中断清除位 */
#define CSR_ECFG_LIE_TI (1UL << 11)  /* 定时器中断启用位 */
#define CSR_ECFG_LIE_HWI0 (1UL << 2) /* 硬件中断0启用位 */
#define CSR_ESTAT_IS_TI (1UL << 11)  /* 定时器中断状态标志位 */
#define CSR_ESTAT_IS_HWI0 (1UL << 2) /* 硬件中断0状态标志位 */

/* CPU 配置 */
#define CC_FREQ 4

/* 内存映射与地址掩码 */
#define DMW_MASK 0x9000000000000000UL            /* 虚拟地址掩码 */
#define L7A_SPACE_BASE (0x10000000UL | DMW_MASK) /* L7A 芯片基础地址 */

/* L7A 中断控制器寄存器 */
#define L7A_INT_MASK (L7A_SPACE_BASE + 0x020)  /* L7A 中断屏蔽寄存器 */
#define L7A_HTMSI_VEC (L7A_SPACE_BASE + 0x200) /* L7A 高级中断向量寄存器 */

/* I/O 控制状态寄存器 (IOCSR) */
#define IOCSR_EXT_IOI_EN 0x1600 /* 外部 I/O 中断启用 */
#define IOCSR_EXT_IOI_SR 0x1800 /* 外部 I/O 中断状态 */

/* 键盘中断相关 */
#define KEYBOARD_IRQ 3                           /* 键盘中断号 */
#define KEYBOARD_IRQ_HT 0                        /* 键盘硬件线索号 */
#define L7A_I8042_DATA (0x1fe00060UL | DMW_MASK) /* 7A桥片寄存器 保存键盘扫描码 */

extern bool cursor_flash; /* 光标闪烁标志 */

void excp_init(void);
void int_on(void);
void exception_handler(void); /* 汇编入口声明 */
void set_timer_period(unsigned int period_ms);
void insert_char_at(ConsoleRegion *reg, int virt_y, int x, char c);

#endif /* EXCEPTION_H */