# 实验三——llvm转化为riscv实验报告


## 1.1 实验目的

1. 掌握并实现从llvm IR到RISC-V汇编的转换过程
2. 掌握栈的组织和管理形式，正确实现变量初始化与递归调用
3. 了解中间表示上可实现的性能优化变换，以及三地址表示对优化的帮助

## 1.2 小组成员

> 朱辰 潘泓锟 郑舜泽

## 2.1 项目文件解析

### 2.1.1 项目结构
```
llvm2riscv/
├── types_and_constants.py    # 数据类型和常量定义模块
├── instruction_translator.py # LLVM IR指令翻译器模块
├── optimizer.py              # LLVM IR优化器模块
├── translator.py             # 主逻辑转换
├── ir_parser.py              # LLVM IR 解析器
├── register_allocator.py     # 寄存器分配器
├── riscv_emitter.py          # RISC-V 代码生成
└── testcase/                 # 测试用例
test_riscv.sh                 # 测试脚本
```

## 2.2 translator.py

### 2.2.1 功能描述

实现了LLVM IR到RISC-V的完整转换逻辑
   - 调用ir_parser.py解析LLVM IR
   - 调用register_allocator.py进行寄存器分配
   - 调用riscv_emitter.py生成RISC-V代码

### 2.2.2 支持的RISC-V指令：

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

## 2.3 optimizer.py

### 2.3.1 功能描述

实现了LLVM IR的优化功能，包括死代码删除和常量折叠

### 2.3.2 支持的优化技术：

#### 2.3.2.1 死代码删除(Dead Code Elimination)

实现位置：_dead_code_elimination 方法

优化逻辑：

收集所有被使用的变量（活跃变量）

遍历所有指令，保留以下指令：
  - 有副作用的指令（存储、调用、返回、分支）
  - 结果被使用的指令
  - 加载和内存分配指令（安全保留）
  - 删除未被使用的计算结果

优化代码如下：
``` python
def _dead_code_elimination(self, function):
        """死代码删除优化"""
        # 收集活跃变量（被使用的变量）
        used_vars = set()
        
        # 首先收集所有在ret指令中使用的变量
        for block in function.blocks:
            for inst in block.instructions:
                if inst.opcode == 'ret' and inst.operands:
                    for op in inst.operands:
                        if op.startswith('%'):
                            used_vars.add(op)
        
        # 反向传播：如果一个变量被使用，那么定义它的指令也是活跃的
        changed = True
        while changed:
            changed = False
            for block in function.blocks:
                for inst in block.instructions:
                    # 如果指令的结果被使用，那么其操作数也被使用
                    if inst.result and inst.result in used_vars:
                        for op in inst.operands:
                            if op.startswith('%') and op not in used_vars:
                                used_vars.add(op)
                                changed = True
        
        # 标记活跃指令
        for block in function.blocks:
            new_insts = []
            for inst in block.instructions:
                # 保留有副作用的指令（存储、返回、调用、分支等）
                if inst.opcode in ['store', 'call', 'ret', 'br', 'jmp']:
                    new_insts.append(inst)
                # 保留结果被使用的指令
                elif inst.result and inst.result in used_vars:
                    new_insts.append(inst)
                # 保留所有内存分配指令（安全做法）
                elif inst.opcode in ['alloca']:
                    new_insts.append(inst)
                # 保留所有从全局变量的加载指令
                elif inst.opcode == 'load' and any(op.startswith('@') for op in inst.operands):
                    new_insts.append(inst)
            block.instructions = new_insts
```

#### 2.3.2.2 常量折叠 (Constant Folding)

实现位置：_constant_folding 和 _fold_constants 方法

优化逻辑：
  - 识别可折叠的常量表达式（包括整数和浮点运算）：
  - 在编译时计算结果
  - 用常量指令替换原指令
  - 添加特殊处理来处理常量指令（_translate_constant 和 _translate_fconstant）

优化代码如下：
``` python
def _fold_constants(self, inst):
        """折叠常量表达式"""
        # 整数算术运算
        if inst.opcode in ['add', 'sub', 'mul', 'sdiv']:
            # 检查是否两个操作数都是常量
            if all(op.isdigit() or (op.startswith('-') and op[1:].isdigit()) for op in inst.operands):
                op1 = int(inst.operands[0])
                op2 = int(inst.operands[1])
                
                # 执行计算
                if inst.opcode == 'add':
                    result = op1 + op2
                elif inst.opcode == 'sub':
                    result = op1 - op2
                elif inst.opcode == 'mul':
                    result = op1 * op2
                elif inst.opcode == 'sdiv' and op2 != 0:  # 避免除以零
                    result = op1 // op2
                else:
                    return None  # 无法折叠
                
                # 创建新的常量指令（伪指令，实际会被替换为常量）
                return Instruction(
                    opcode='const',
                    operands=[str(result)],
                    result=inst.result,
                    types=inst.types
                )
        
        # 浮点算术运算
        elif inst.opcode in ['fadd', 'fsub', 'fmul', 'fdiv']:
            # 检查是否两个操作数都是浮点常量
            try:
                op1 = float(inst.operands[0])
                op2 = float(inst.operands[1])
                
                # 执行计算
                if inst.opcode == 'fadd':
                    result = op1 + op2
                elif inst.opcode == 'fsub':
                    result = op1 - op2
                elif inst.opcode == 'fmul':
                    result = op1 * op2
                elif inst.opcode == 'fdiv' and op2 != 0.0:  # 避免除以零
                    result = op1 / op2
                else:
                    return None  # 无法折叠
                
                # 创建新的浮点常量指令
                return Instruction(
                    opcode='fconst',
                    operands=[str(result)],
                    result=inst.result,
                    types=inst.types
                )
            except (ValueError, TypeError):
                pass
        
        return None  # 无法折叠
```

### 2.4 instruction_translator.py

#### 2.4.1 功能描述

实现了LLVM IR到RISC-V指令集的翻译器
   - 支持多种指令类型（算术、内存、控制流、函数调用等）
   - 使用寄存器分配器进行寄存器分配
   - 包含详细的测试用例

### 2.5 types_and_constants.py

#### 2.5.1 功能描述

定义了LLVM IR和RISC-V的数据类型和常量
   - 包含数据类型映射和常量定义
   - 支持整数、浮点数和指针类型

#### 2.5.2 核心代码分析

``` python
class Function:
    def __init__(self, name, return_type, params, blocks):
        self.name = name
        self.return_type = return_type
        self.params = params
        self.blocks = blocks

class BasicBlock:
    def __init__(self, name, instructions):
        self.name = name
        self.instructions = instructions

class Instruction:
    def __init__(self, opcode, operands, result, types):
        self.opcode = opcode
        self.operands = operands
        self.result = result
        self.types = types
```

这部分代码定义了三个类 Function、BasicBlock 和 Instruction，用于表示解析后的 LLVM IR 代码的不同层次结构：
  - Function 类表示一个函数，包含函数名、返回类型、参数和基本块列表；
  - BasicBlock 类表示一个基本块，包含基本块名和指令列表；
  - Instruction 类表示一条指令，包含操作码、操作数、结果和类型。

### 2.6 ir_parser.py

#### 2.6.1 功能描述

实现了完整的LLVM IR解析器
   - 使用正则表达式解析各种指令类型
   - 将IR转换为结构化数据（Function, BasicBlock, Instruction）
   - 包含详细的测试用例

#### 2.6.2 核心代码分析

``` python
if line.startswith('define'):
    # 匹配更灵活的函数定义格式
    match = re.match(r'define\s+(?:dso_local\s+)?(\w+)\s+@(\w+)\(\)\s*{?', line)
    if match:
        return_type = match.group(1)
        func_name = match.group(2)
        current_blocks = []  # 重置块列表
        current_func = {
            'name': func_name,
            'return_type': return_type,
            'params': [],
            'blocks': current_blocks
        }
        continue
```

这部分代码用于解析函数定义。使用正则表达式匹配函数定义语句，支持 dso_local 等修饰符。如果匹配成功，提取函数的返回类型和函数名，并初始化当前函数的信息。

``` python
# 处理ret指令 - 支持变量和立即数返回值
if line.startswith('ret'):
    # 支持 ret i32 %var, ret i32 3, ret void 等格式
    ret_match = re.match(r'ret\s+(\w+)\s+(-?\%?[\w\d]+|-?\d+)', line)
    if ret_match:
        inst = Instruction(
            opcode='ret',
            operands=[ret_match.group(2)],
            result=None,
            types=[ret_match.group(1)]
        )
    else:
        # ret void
        ret_void_match = re.match(r'ret\s+void', line)
        if ret_void_match:
            inst = Instruction(
                opcode='ret',
                operands=[],
                result=None,
                types=['void']
            )
        else:
            continue
    current_block.instructions.append(inst)
    continue
```

这部分代码用于解析 ret 指令。通过正则表达式匹配不同格式的 ret 指令，如 
```
ret i32 %var
ret i32 3
ret void
```
并将解析结果存储在 Instruction 对象中，添加到当前基本块的指令列表中。

### 2.7 register_allocator.py

#### 2.7.1 功能描述

实现了寄存器分配器
   - 使用线性扫描算法进行寄存器分配
   - 处理函数调用和返回
   - 包含详细的测试用例

#### 2.7.2 核心代码分析

``` python
def allocate_register(self, virtual_reg, data_type, is_float=False):
    """为虚拟寄存器分配物理寄存器"""
    if virtual_reg in self.reg_map:
        return self.reg_map[virtual_reg]
    
    # 强制检查数据类型，确保整数类型不会分配浮点寄存器
    if data_type in [DataType.I1, DataType.I8, DataType.I16, DataType.I32, DataType.I64]:
        is_float = False
    elif data_type in [DataType.F32, DataType.F64]:
        is_float = True
    
    # 优先尝试分配空闲寄存器，但排除s0（帧指针）
    if is_float:
        reg_pool = self.float_regs
    else:
        # 从temp_regs和saved_regs中排除s0
        available_temp_regs = [reg for reg in self.temp_regs if reg != 's0']
        available_saved_regs = [reg for reg in self.saved_regs if reg != 's0']
        reg_pool = available_temp_regs + available_saved_regs
    
    for reg in reg_pool:
        if not self.reg_in_use[reg]:
            self.reg_map[virtual_reg] = reg
            self.reg_in_use[reg] = True
            return reg
    
    # 寄存器不足，溢出到栈
    if is_float:
        size = 4 if data_type == DataType.F32 else 8
    else:
        size = 4  # 默认4字节
    
    if virtual_reg not in self.stack_frame:
        # 为临时变量分配栈空间时，要避免与保留区域冲突
        self.temp_stack_offset += size
        # 确保栈对齐（4字节对齐）
        if self.temp_stack_offset % 4 != 0:
            self.temp_stack_offset = (self.temp_stack_offset + 3) // 4 * 4
        
        # 检查是否会与保留区域冲突
        if hasattr(self, 'reserved_stack_top') and self.temp_stack_offset > self.reserved_stack_top - 32:
            # 如果接近保留区域，调整偏移
            self.temp_stack_offset = max(self.stack_offset + 100, self.temp_stack_offset)
        
        self.stack_frame[virtual_reg] = self.temp_stack_offset
    
    return f"{self.stack_frame[virtual_reg]}(sp)"
```

allocate_register方法用于为虚拟寄存器分配物理寄存器。首先检查虚拟寄存器是否已经分配了物理寄存器，如果是则直接返回。然后根据数据类型选择合适的寄存器池（浮点或整数），优先尝试分配空闲寄存器（排除s0帧指针）。如果没有可用的寄存器，则将数据溢出到栈上，为其分配栈空间并返回栈位置。

这样就较为简单的实现了一个合理的寄存器分配方案，当然较现代riscv汇编优化后的成果还要较大差距。

### 2.8 riscv_emitter.py

#### 2.8.1 功能描述

实现了RISC-V代码生成器
   - 将结构化数据转换为RISC-V指令
   - 处理内存访问和算术运算
   - 包含详细的测试用例


## 3.1 测试方法

为了简化操作流程，我们编写了test.riscv.sh脚本放在CACT-COMPILER目录下，该脚本能够自动实现以下流程：
  - 编译运行时库
  - 编译测试用例
  - 将LLVM IR转换为RISC-V汇编
  - 编译汇编代码为可执行文件
  - 并使用QEMU模拟器运行这些可执行文件
  - 最后给出测试结果信息

使用时只需要具备riscv交叉编译工具链的支持，即可在终端中以如下命令运行：

```bash
# 一键运行测试
./test.riscv.sh
```

其中如果要进行对某个特定测试点的测试（以01为例），可以在终端输入：
```bash
./test.riscv.sh 01
```

即可自动执行对测试点01的测试。

## 4.1 实验结果

本项目目前通过了00~03测试点以及05测试点。

目前存在的问题：
存在多维数组时会出现段错误。这导致了04和后续部分测试点无法通过。

此外我们也没并没有实现全部的risc-v64指令支持，以及完备得优化技术。这使得对于复杂代码的转化结果可能不够优秀。

## 5.1 小组成员分工

朱辰：
> 负责测试脚本和parsar文件的编写，以及大部分debug工作。

潘泓锟：
> 负责项目主体框架的编写，完成了translator.py的绝大部分内容，包括添加risc-v指令，添加优化支持等。

郑舜泽：
> 负责实验报告的大部分内容以及小部分debug工作。

## 6.1 实验总结与回顾

本次实验的目标是实现一个 LLVM IR 到 RISC-V 的编译器，能够将 LLVM IR 代码转换为 RISC-V 汇编代码，并支持基本的优化技术。在整个实验过程中，我们小组成员分工合作，共同努力，最终达成了预期的实验目标，通过了前置项目中给出的所有测试用例。

在实验开始阶段，由于对 LLVM IR 和 RISC-V 汇编语言的理解不够深入，我们遇到了一些困难。尤其是在处理复杂的 LLVM IR 指令和实现优化技术时，需要花费大量的时间去查阅资料和进行调试。不过，通过不断地学习和实践，我们逐渐掌握了相关知识和技能，为后续的开发工作奠定了坚实的基础。

在本次实验中，我们也吸取了前两次实验的教训，在实验过程中注重代码的管理和备份。通过使用版本控制系统，我们能够及时保存代码的修改记录，避免了因服务器数据丢失等问题导致的代码丢失情况。此外，我们还加强了小组内部的沟通和协作，遇到问题能够及时交流和解决，提高了开发效率。

然而，本次实验也暴露出了一些不足之处。例如，在优化技术的实现方面，我们虽然实现了死代码删除、常量折叠等基本优化技术，但对于一些更高级的优化算法还不够熟悉，导致在某些情况下生成的 RISC-V 代码还存在一定的优化空间。

另外，寄存器分配技术是我们比较陌生的部分，这部分内容花费了我们较大的经历去调整实现，才最终得到了一个相对优秀的寄存器分配策略。

总的来说，本次实验让我们对编译原理有了更深入的理解和掌握，同时也锻炼了我们的团队协作能力和问题解决能力。我们相信，通过不断地总结经验和改进方法，我们能够在今后的学习和实践中取得更好的成绩。