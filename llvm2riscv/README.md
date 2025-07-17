# llvm转化为riscv工程结构

## 文件结构如下

llvm2riscv/
├── translator.py            # 主转换逻辑
├── ir_parser.py             # LLVM IR 解析器
├── register_allocator.py    # 寄存器分配器
├── riscv_emitter.py         # RISC-V 代码生成
└── testcase/                   # 测试用例

### translator.py

实现了LLVM IR到RISC-V的完整转换逻辑
   - 调用ir_parser.py解析LLVM IR
   - 调用register_allocator.py进行寄存器分配
   - 调用riscv_emitter.py生成RISC-V代码

现在支持的RISC-V指令：

内存管理类：

>LB LBU LH LHU LW LWU LUI LWU SB SH SW

算术类：

>ADDI ADDIW ADD SUB SUBW AND OR XOR SLL SRL SRA

移位类：

>SLLI SRLI SRAI

比较类：

>SLTI SLTIU SLT SLTU

跳转类：

>JAL JALR BEQ BNE BLT BGE BLTU BGEU


### ir_parser.py

实现了完整的LLVM IR解析器
   - 使用正则表达式解析各种指令类型
   - 将IR转换为结构化数据（Function, BasicBlock, Instruction）
   - 包含详细的测试用例

### register_allocator.py

实现了寄存器分配器
   - 使用线性扫描算法进行寄存器分配
   - 处理函数调用和返回
   - 包含详细的测试用例

### riscv_emitter.py

实现了RISC-V代码生成器
   - 将结构化数据转换为RISC-V指令
   - 处理内存访问和算术运算
   - 包含详细的测试用例

### testcase/

包含了一系列LLVM IR测试用例，用于验证转换逻辑的正确性

## 使用方法 