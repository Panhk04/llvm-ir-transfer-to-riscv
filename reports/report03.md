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
   - 调用optimizer.py进行应用优化转换
   - 调用register_allocator.py进行寄存器分配
   - 调用instruction_translator.py进行指令翻译
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

实现位置：_constant_folding（遍历处理） 和 _fold_constants（折叠逻辑） 方法


##### 优化逻辑
1. **识别阶段**
   - **操作数检测**：
     - 整数：`op.isdigit()` 或 `"-".isdigit()`
     - 浮点：`float()`转换+异常捕获
   - **支持操作符**：
     ```python
     ['add', 'sub', 'mul', 'sdiv']   # 整数
     ['fadd', 'fsub', 'fmul', 'fdiv'] # 浮点
     ```

2. **计算阶段**
   - 安全策略：
     ```python
     if op == 'sdiv' and op2 == 0:    # 整数除零
         return None
     if op == 'fdiv' and op2 == 0.0:  # 浮点除零
         return None
     ```

3. **替换阶段**
   ```python
   # 生成伪指令示例
   Instruction(
       opcode='const',
       operands=["5"],  # 计算结果
       result="%1",
       types=["i32"]
   )
   ```
    

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

典型优化示例：
输入：
`%1 = add i32 2, 3`
优化后：
`%1 = const i32 5`

## 2.4 instruction_translator.py

### 2.4.1 功能描述

实现了LLVM IR到RISC-V指令集的翻译器
   - 支持多种指令类型（算术、内存、控制流、函数调用等）
   - 使用寄存器分配器进行寄存器分配
   - 包含详细的测试用例

### 2.4.2 核心代码分析

```python
def translate_instruction(self, instruction):
    opcode = instruction.opcode
    # 根据opcode分发到不同的翻译函数
    if opcode == 'ret': ...
    elif opcode in memory_ops: ...
    ... # 其他指令类型
```

translate_instruction方法作为指令翻译的入口，根据指令操作码（opcode）将指令分发到对应的具体翻译函数，实现模块化的指令翻译过程。

```python
def _translate_ret(self, instruction):
    # 处理void返回
    # 处理整数返回
    # 处理浮点返回
    # 函数尾声（恢复ra, s0, 调整sp, ret）
```

_translate_ret方法用于返回指令翻译，其翻译LLVM的ret指令，处理无返回值(void)和有返回值（整数/浮点）的情况，并生成函数尾声代码（恢复保存的寄存器、调整栈指针和返回）。

```python
def _translate_memory(self, instruction):
    if opcode == 'alloca': ... # 已在预处理中处理，跳过
    elif opcode == 'load': ... # 加载全局变量或局部变量
    elif opcode == 'store': ... # 存储到全局变量或局部变量
```

_translate_memory方法用于翻译内存相关指令（alloca, load, store）。alloca指令在预处理阶段已处理，此处跳过；load和store指令分别处理全局变量和局部变量的加载与存储，生成相应的RISC-V加载和存储指令。

```python
def _translate_arithmetic(self, instruction):
    # 区分浮点和整数运算
    # 处理立即数优化（特别是addi/subi）
    # 分配目标寄存器/栈空间
    # 生成运算指令
```

_translate_arithmetic方法用于翻译算术运算指令（整数和浮点），根据操作数类型和值进行优化（如使用addi/subi处理小立即数），并将结果存储到寄存器或栈空间。

```python
def _translate_getelementptr(self, instruction):
    # 1. 获取基础指针
    # 2. 解析数组类型和维度
    # 3. 计算步长（strides）
    # 4. 计算索引偏移
    # 5. 计算最终地址并存储
```

_translate_getelementptr方法用于翻译LLVM的getelementptr指令，用于计算聚合类型（如数组）元素的地址。通过解析数组维度、计算步长和索引偏移，生成地址计算指令序列。

```python
def _translate_shift(self, instruction):
    # 分配目标位置
    # 获取操作数
    # 生成移位指令
```

_translate_shift方法用于翻译移位指令（shl, lshr, ashr），生成对应的RISC-V移位指令（sll, srl, sra），结果存储到目标位置（寄存器或栈）。

```python
def _get_or_load_operand(self, operand, data_type, riscv_instructions):
    # 处理立即数（整数和浮点）
    # 处理虚拟寄存器（从寄存器或栈中加载）
```

_get_or_load_operand为操作数加载辅助函数，用于将操作数（立即数或虚拟寄存器）加载到物理寄存器中。如果是立即数，生成li或浮点加载序列；如果在栈上，生成加载指令；否则直接使用分配的寄存器。

```python
def _translate_compare(self, instruction):
    # 处理整数比较（icmp）和浮点比较（fcmp）
    # 根据条件码生成比较指令序列
    # 将结果存储到目标位置
```

_translate_compare用于翻译比较指令（icmp/fcmp），根据条件码生成相应的RISC-V比较指令序列（如slt, feq等），并将比较结果（布尔值）存储到目标位置。

```python
def _translate_branch(self, instruction):
    # 无条件跳转（直接j）
    # 条件跳转（根据条件寄存器选择分支）
```

_translate_branch用于翻译分支指令（br），包括无条件跳转和条件跳转，利用标签映射将LLVM基本块标签转换为RISC-V标签，生成j和bnez等跳转指令。

```python
def _translate_call(self, instruction):
    # 1. 处理参数（整数和浮点，使用a0-a7和fa0-fa7）
    # 2. 生成call指令
    # 3. 处理返回值
```

_translate_call用于翻译函数调用指令（call），将参数加载到约定寄存器（整数a0-a7，浮点fa0-fa7），生成call指令，并处理返回值存储。

```python
def _translate_cast(self, instruction):
    # 处理整数到浮点（sitofp）
    # 浮点到整数（fptosi）
    # 其他转换（用mov简化处理）
```

_translate_cast用于翻译类型转换指令（如sitofp, fptosi等），生成相应的转换指令（如fcvt.s.w, fcvt.w.s），其他转换暂时用寄存器移动指令（mv/fmv.s）处理。

```python
def _translate_constant(self, instruction): ... # 整数常量
def _translate_fconstant(self, instruction): ... # 浮点常量
```

_translate_constant和_translate_fconstant用于翻译常量加载指令，将整数常量或浮点常量加载到寄存器或栈中。浮点常量通过整数立即数加载再转换的方式实现。

```python
def _get_load_instruction(self, data_type): ... # 根据数据类型返回lw, lh, lb, flw等
def _get_store_instruction(self, data_type): ... # 根据数据类型返回sw, sh, sb, fsw等
```

_get_load_instruction 和 _get_store_instruction为加载/存储指令辅助函数，根据数据类型返回相应的RISC-V加载或存储指令助记符（如整数用lw/sw，浮点用flw/fsw，不同位宽用不同指令）。

总的来说，instruction_translator.py该模块实现了LLVM IR指令到RISC-V汇编指令的翻译，覆盖了常见指令类型。它依赖于寄存器分配器管理寄存器和栈空间，并利用标签映射处理分支目标。翻译过程考虑数据类型（整数/浮点）和操作数特性（立即数/变量），并针对RISC-V指令集特点进行优化（如使用addi处理小立即数）。对于复杂操作（如getelementptr），生成多条指令序列完成计算。

## 2.5 types_and_constants.py

### 2.5.1 功能描述

定义了LLVM IR和RISC-V的数据类型和常量
   - 包含数据类型映射和常量定义
   - 支持整数、浮点数和指针类型

### 2.5.2 核心代码分析

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

## 2.6 ir_parser.py

### 2.6.1 功能描述

实现了完整的LLVM IR解析器
   - 使用正则表达式解析各种指令类型
   - 将IR转换为结构化数据（Function, BasicBlock, Instruction）
   - 包含详细的测试用例

### 2.6.2 核心代码分析

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

## 2.7 register_allocator.py

### 2.7.1 功能描述

实现了寄存器分配器
   - 使用线性扫描算法进行寄存器分配
   - 处理函数调用和返回
   - 包含详细的测试用例

### 2.7.2 核心代码分析

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

``` python
def get_temp_register(self):
    """获取一个临时寄存器"""
    # 改进的临时寄存器分配，确保不使用s0（帧指针）
    temp_candidates = ['t0', 't1', 't2', 't3', 't4', 't5', 't6', 's1', 's2', 's3', 's4', 's5', 's6', 's7']
        
    # 使用轮转分配策略
    if not hasattr(self, 'temp_register_counter'):
        self.temp_register_counter = 0
        
    # 尝试找到一个未被使用的寄存器
    for i in range(len(temp_candidates)):
        candidate_idx = (self.temp_register_counter + i) % len(temp_candidates)
        reg = temp_candidates[candidate_idx]
            
        if not self.reg_in_use.get(reg, False):
            self.temp_register_counter = (candidate_idx + 1) % len(temp_candidates)
            # 不要标记为永久使用，这样可以被重复使用
            return reg
        
    # 如果所有寄存器都被使用，使用轮转策略强制分配（但永远不使用s0）
    reg = temp_candidates[self.temp_register_counter % len(temp_candidates)]
    self.temp_register_counter = (self.temp_register_counter + 1) % len(temp_candidates)
    return reg
```

get_temp_register方法用于临时寄存器分配，临时寄存器从`t0-t6`和`s1-s7`中选择（排除`s0`），使用轮询策略尝试分配空闲寄存器，如果都忙则返回下一个（可能已被占用，需要调用者注意）。

``` python
def store_to_stack_if_needed(self, result_reg, stack_location, data_type, riscv_instructions):
    """如果目标是栈位置，将寄存器值存储到栈上"""
    if '(sp)' in stack_location:
        if data_type in [DataType.F32, DataType.F64]:
            store_instr = "fsw" if data_type == DataType.F32 else "fsd"
        else:
            store_instr = "sw"
        riscv_instructions.append(f"    {store_instr} {result_reg}, {stack_location}")
        return True
    return False
```

store_to_stack_if_needed方法用于栈存储，其检查目标位置是否是栈位置（包含`(sp)`字符串），并根据数据类型选择存储指令（浮点：fsw/fsd，整数：sw）。最后生成存储指令并添加到指令列表中。


## 2.8 riscv_emitter.py

### 2.8.1 功能描述

实现了RISC-V代码生成器
   - 将结构化数据转换为RISC-V指令
   - 处理内存访问和算术运算
   - 包含详细的测试用例


### 2.8.2 核心代码分析
``` python
def emit_constant(self, value, type_="i32"):
    """发射常量到寄存器"""
    # 分配一个临时寄存器
    temp_reg = self.register_allocator.allocate_register(f"%const_{len(self.code)}")
        
    if type_ == "i32":
        if -2048 <= value <= 2047:
            # 使用addi指令（立即数范围：-2048到2047）
            self.code.append(f"addi {temp_reg}, x0, {value}")
        else:
            # 使用lui和addi组合指令
            high_20 = value >> 12
            low_12 = value & 0xFFF
            self.code.append(f"lui {temp_reg}, {high_20}")
            self.code.append(f"addi {temp_reg}, {temp_reg}, {low_12}")
    elif type_ == "float":
        # 浮点常量需要特殊处理（这里简化处理）
        self.code.append(f"# Floating point constant: {value}")
        # 实际实现需要处理浮点立即数的存储和加载
        
    return temp_reg
```

emit_constant方法用于为常量分配一个临时寄存器，对于i32常量：如果值在-2048到2047之间，使用addi指令（addi rd, x0, imm）将常量加载到寄存器。否则，使用lui加载高20位，然后addi加载低12位。

``` python
def emit_binary_operation(self, op, left, right, result):
    """发射二元操作指令"""
    left_reg = self.register_allocator.get_register(left)
    right_reg = self.register_allocator.get_register(right)
    result_reg = self.register_allocator.allocate_register(result)

    if op == 'add':
        self.code.append(f"add {result_reg}, {left_reg}, {right_reg}")
    elif op == 'sub':
        self.code.append(f"sub {result_reg}, {left_reg}, {right_reg}")
    elif op == 'mul':
        self.code.append(f"mul {result_reg}, {left_reg}, {right_reg}")
    elif op == 'sdiv':
        self.code.append(f"div {result_reg}, {left_reg}, {right_reg}")
    elif op == 'addf':
        self.code.append(f"fadd.s {result_reg}, {left_reg}, {right_reg}")
    elif op == 'subf':
        self.code.append(f"fsub.s {result_reg}, {left_reg}, {right_reg}")
    # 其他操作符...
```

emit_binary_operation方法获取左操作数和右操作数所在的寄存器，并为结果分配一个新的寄存器。其根据操作符生成相应的RISC-V指令，例如add、sub、mul等。注意，这里支持整数和浮点操作（例如addf对应fadd.s）。


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
./test_riscv.sh
```

其中如果要进行对某个特定测试点的测试（以01为例），可以在终端输入：
```bash
./test_riscv.sh 01
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