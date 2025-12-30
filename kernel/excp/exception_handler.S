# 宏定义：保存/恢复寄存器的通用操作
# 参数 cmd：可以是 st.d（存储）或 ld.d（加载）
# st.d $ra, $sp, 0x0   内存[$sp + 0x0] = $ra  # 把 $ra 的 8 字节内容写入该地址
# ld.d $ra, $sp, 0x0   $ra = 内存[$sp + 0x0]  # 从该地址读取 8 字节到 $ra
.macro store_load_regs cmd #八字节偏移
    \cmd $ra, $sp, 0x0      # 返回地址寄存器（保存函数返回地址）
    \cmd $tp, $sp, 0x8      # 线程指针寄存器
    \cmd $a0, $sp, 0x10     # 参数寄存器 0（函数第1个参数）
    \cmd $a1, $sp, 0x18     # 参数寄存器 1（函数第2个参数）
    \cmd $a2, $sp, 0x20     # 参数寄存器 2
    \cmd $a3, $sp, 0x28     # 参数寄存器 3
    \cmd $a4, $sp, 0x30     # 参数寄存器 4
    \cmd $a5, $sp, 0x38     # 参数寄存器 5
    \cmd $a6, $sp, 0x40     # 参数寄存器 6
    \cmd $a7, $sp, 0x48     # 参数寄存器 7
    \cmd $t0, $sp, 0x50     # 临时寄存器 0（调用者保存）
    \cmd $t1, $sp, 0x58     # 临时寄存器 1
    \cmd $t2, $sp, 0x60     # 临时寄存器 2
    \cmd $t3, $sp, 0x68     # 临时寄存器 3
    \cmd $t4, $sp, 0x70     # 临时寄存器 4
    \cmd $t5, $sp, 0x78     # 临时寄存器 5
    \cmd $t6, $sp, 0x80     # 临时寄存器 6
    \cmd $t7, $sp, 0x88     # 临时寄存器 7
    \cmd $t8, $sp, 0x90     # 临时寄存器 8
    \cmd $r21, $sp,0x98     # 通用寄存器 21
    \cmd $fp, $sp, 0xa0     # 帧指针寄存器（栈帧基址）
    \cmd $s0, $sp, 0xa8     # 保存寄存器 0（被调用者保存）
    \cmd $s1, $sp, 0xb0     # 保存寄存器 1
    \cmd $s2, $sp, 0xb8     # 保存寄存器 2
    \cmd $s3, $sp, 0xc0     # 保存寄存器 3
    \cmd $s4, $sp, 0xc8     # 保存寄存器 4
    \cmd $s5, $sp, 0xd0     # 保存寄存器 5
    \cmd $s6, $sp, 0xd8     # 保存寄存器 6
    \cmd $s7, $sp, 0xe0     # 保存寄存器 7
    \cmd $s8, $sp, 0xe8     # 保存寄存器 8
.endm                       # 宏定义结束

    .globl exception_handler    # 声明全局符号，供链接器使用
exception_handler:              # 异常处理入口点
    addi.d $sp, $sp, -0xf0      # 分配栈空间 240 字节（0xf0 = 240），sp向低地址移动240字节
    
    store_load_regs st.d         # 调用宏，使用 st.d 指令保存所有寄存器到栈，sp向高地址增长
                                 
    #在中断中会改变寄存器数值，结束后从内存中恢复
    bl do_exception              # 跳转并链接到 C 函数 do_exception
                                 # bl = branch and link（保存返回地址到 $ra）
																 # do_exception返回时pc=$ra指向store_load_regs ld.d

    store_load_regs ld.d         # 调用宏，使用 ld.d 指令从栈恢复所有寄存器
																 #恢复所有寄存器到中断前的数值，包括ra，但是pc寄存器不变，继续执行下面两条指令
    
    addi.d $sp, $sp, 0xf0        # 释放栈空间（恢复栈指针）
    
    ertn                         # 异常返回指令（从异常处理返回到中断点）
                                 # ertn = exception return
																 # pc=ERA 跳转到异常点 ERA保存异常发生点地址

#ERA	ERA 保存"中断发生的位置"
#$ra	保存"中断发生时，当前函数应该返回到的位置"