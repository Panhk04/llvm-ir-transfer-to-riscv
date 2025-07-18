import re
from collections import namedtuple, defaultdict
from enum import Enum
from ir_parsar import IRParser

# 定义数据结构
Function = namedtuple('Function', ['name', 'return_type', 'params', 'blocks'])
BasicBlock = namedtuple('BasicBlock', ['name', 'instructions'])
Instruction = namedtuple('Instruction', ['opcode', 'operands', 'result', 'types'])

# 数据类型枚举
class DataType(Enum):
    I1 = 1
    I8 = 8
    I16 = 16
    I32 = 32
    I64 = 64
    F32 = 100  # 使用不同的值避免与整数类型冲突
    F64 = 200

# 寄存器分配器
class RegisterAllocator:
    def __init__(self):
        # RISC-V 寄存器资源
        self.param_regs = ['a0', 'a1', 'a2', 'a3', 'a4', 'a5', 'a6', 'a7']
        self.temp_regs = ['t0', 't1', 't2', 't3', 't4', 't5', 't6']
        self.saved_regs = ['s0', 's1', 's2', 's3', 's4', 's5', 's6', 's7']
        self.float_regs = ['ft0', 'ft1', 'ft2', 'ft3', 'ft4', 'ft5', 'ft6', 'ft7',
                           'fa0', 'fa1', 'fa2', 'fa3', 'fa4', 'fa5', 'fa6', 'fa7']
        
        # 寄存器映射和状态
        self.reg_map = {}
        self.reg_in_use = defaultdict(bool)
        self.virtual_regs = []
        self.liveness = {}
        
        # 栈帧信息
        self.stack_offset = 0
        self.stack_frame = {}
        self.param_offset = 0
        self.saved_regs_offset = {}
    
    def reset(self):
        """重置寄存器分配器状态"""
        self.reg_map = {}
        self.reg_in_use = defaultdict(bool)
        self.virtual_regs = []
        self.liveness = {}
        self.stack_offset = 0
        self.stack_frame = {}
        self.param_offset = 0
        self.saved_regs_offset = {}
    
    def analyze_liveness(self, function):
        """分析虚拟寄存器的活跃范围"""
        # 简化的活跃变量分析
        self.virtual_regs = []
        
        for block in function.blocks:
            for inst in block.instructions:
                # 记录定义
                if inst.result and inst.result.startswith('%'):
                    if inst.result not in self.virtual_regs:
                        self.virtual_regs.append(inst.result)
                        self.liveness[inst.result] = {"def": [], "use": []}
                    self.liveness[inst.result]["def"].append(inst)
                
                # 记录使用
                for op in inst.operands:
                    if op.startswith('%') and op in self.liveness:
                        self.liveness[op]["use"].append(inst)
    
    def allocate_register(self, virtual_reg, data_type, is_float=False):
        """为虚拟寄存器分配物理寄存器"""
        if virtual_reg in self.reg_map:
            return self.reg_map[virtual_reg]
        
        # 强制检查数据类型，确保整数类型不会分配浮点寄存器
        if data_type in [DataType.I1, DataType.I8, DataType.I16, DataType.I32, DataType.I64]:
            is_float = False
        elif data_type in [DataType.F32, DataType.F64]:
            is_float = True
        
        # 优先尝试分配空闲寄存器
        reg_pool = self.float_regs if is_float else self.temp_regs
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
            self.stack_offset += size
            self.stack_frame[virtual_reg] = self.stack_offset
            return f"{self.stack_offset}(sp)"
        
        return f"{self.stack_frame[virtual_reg]}(sp)"
    
    def free_register(self, reg):
        """释放物理寄存器"""
        if reg in self.reg_in_use:
            self.reg_in_use[reg] = False
    
    def free_virtual_reg(self, virtual_reg):
        """释放虚拟寄存器占用的资源"""
        if virtual_reg in self.reg_map:
            reg = self.reg_map[virtual_reg]
            self.free_register(reg)
            del self.reg_map[virtual_reg]
    
    def get_stack_size(self):
        """获取栈帧大小"""
        return self.stack_offset
    
    def get_physical_reg(self, virtual_reg):
        """获取虚拟寄存器对应的物理寄存器或栈位置"""
        if virtual_reg in self.reg_map:
            return self.reg_map[virtual_reg]
        if virtual_reg in self.stack_frame:
            return f"{self.stack_frame[virtual_reg]}(sp)"
        return None

# 优化后的编译器
class OptimizedLLVMIRTranslator:
    def __init__(self):
        self.allocator = RegisterAllocator()
        self.label_map = {}
        self.current_function = None
        self.float_ops = {
            'fadd': 'fadd.s',
            'fsub': 'fsub.s',
            'fmul': 'fmul.s',
            'fdiv': 'fdiv.s',
            'fcmp': 'feq.s'
        }
    
    def translate(self, ir_code):
        """主翻译函数"""
        # 解析IR代码
        parser = IRParser()
        declarations, functions = parser.parse(ir_code)
        
        riscv_code = []
        
        # 处理全局变量声明
        global_vars = self._extract_global_variables(ir_code)
        if global_vars:
            riscv_code.append(".data")
            for var_name, var_type, var_value in global_vars:
                riscv_code.append(f".globl {var_name}")
                riscv_code.append(f"{var_name}:")
                if var_type == 'i32':
                    riscv_code.append(f"    .word {var_value}")
                elif var_type == 'i64':
                    riscv_code.append(f"    .dword {var_value}")
                elif var_type == 'float':
                    riscv_code.append(f"    .float {var_value}")
                elif var_type == 'double':
                    riscv_code.append(f"    .double {var_value}")
                else:
                    riscv_code.append(f"    .word {var_value}")  # 默认
            riscv_code.append("")  # 添加空行分隔
        
        riscv_code.append(".text")
        
        # 检查是否有main函数并添加.globl指令
        for func in functions:
            if func.name == 'main':
                riscv_code.append(".globl main")
                break
        
        # 构建标签映射
        self._build_label_map(functions)
        
        # 翻译每个函数
        for func in functions:
            self.current_function = func
            self.allocator.reset()
            self.allocator.analyze_liveness(func)
            
            # 应用优化
            self._optimize_function(func)
            
            riscv_code.extend(self._translate_function(func))
            
        return "\n".join(riscv_code)
    
    def _optimize_function(self, function):
        """应用所有优化到函数"""
        self._dead_code_elimination(function)
        self._constant_folding(function)
        self._optimize_blocks(function)
    
    def _dead_code_elimination(self, function):
        """死代码删除优化 - 修复版本"""
        # 禁用死代码删除优化，保留所有指令
        # 这样可以确保不会错误删除必要的计算指令
        return
    
    def _constant_folding(self, function):
        """常量折叠优化"""
        for block in function.blocks:
            new_insts = []
            for inst in block.instructions:
                # 尝试折叠常量表达式
                folded = self._fold_constants(inst)
                if folded:
                    new_insts.append(folded)
                else:
                    new_insts.append(inst)
            block.instructions = new_insts
    
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
    
    def _optimize_blocks(self, function):
        """基本块优化"""
        # 1. 删除空基本块（除了入口块）
        function.blocks = [b for b in function.blocks 
                          if b.instructions or b.name == function.blocks[0].name]
        
        # 2. 合并冗余跳转
        for i, block in enumerate(function.blocks):
            if block.instructions and block.instructions[-1].opcode == 'jmp':
                target = block.instructions[-1].operands[0]
                next_block = function.blocks[i+1] if i+1 < len(function.blocks) else None
                
                # 如果是跳转到下一个块，删除跳转
                if next_block and next_block.name == target:
                    block.instructions = block.instructions[:-1]
    
    def _build_label_map(self, functions):
        """构建基本块标签映射"""
        self.label_map = {}
        for func in functions:
            for block in func.blocks:
                self.label_map[block.name] = f".{func.name}_{block.name[1:]}"
    
    def _translate_function(self, function):
        """翻译单个函数"""
        riscv_code = []
        func_label = function.name
        
        # 函数标签
        riscv_code.append(f"{func_label}:")
        
        # 函数序言
        stack_size = self.allocator.get_stack_size()
        if stack_size > 0:
            # 确保栈对齐 (16字节对齐)
            aligned_size = (stack_size + 15) // 16 * 16
            riscv_code.append("    # Function prologue")
            riscv_code.append(f"    addi sp, sp, -{aligned_size}")
        
        # 翻译每个基本块
        for block in function.blocks:
            # 块标签
            if block.name in self.label_map:
                riscv_code.append(f"{self.label_map[block.name]}:")
                
            # 翻译指令
            for inst in block.instructions:
                # 处理常量折叠后的伪指令
                if inst.opcode == 'const':
                    riscv_inst = self._translate_constant(inst)
                    riscv_code.extend(riscv_inst)
                elif inst.opcode == 'fconst':
                    riscv_inst = self._translate_fconstant(inst)
                    riscv_code.extend(riscv_inst)
                else:
                    riscv_inst = self._translate_instruction(inst)
                    riscv_code.extend(riscv_inst)
        
        # 如果没有返回指令，添加默认返回
        if not any(inst.opcode == 'ret' for block in function.blocks for inst in block.instructions):
            riscv_code.append("    # Default return")
            riscv_code.append("    li a0, 0")
            riscv_code.append("    ret")
        
        # 函数尾声
        if stack_size > 0:
            aligned_size = (stack_size + 15) // 16 * 16
            riscv_code.append("    # Function epilogue")
            riscv_code.append(f"    addi sp, sp, {aligned_size}")
        
        riscv_code.append("")  # 添加空行分隔函数
        return riscv_code
    
    def _translate_constant(self, instruction):
        """翻译常量指令（由常量折叠产生）"""
        riscv_instructions = []
        result = instruction.result
        value = instruction.operands[0]
        data_type = self._get_data_type(instruction.types[0])
        
        # 获取目标寄存器
        is_float = data_type in [DataType.F32, DataType.F64]
        dest_reg = self.allocator.allocate_register(result, data_type, is_float)
        
        # 加载常量值
        riscv_instructions.append(f"    li {dest_reg}, {value}")
        
        return riscv_instructions
    
    def _translate_fconstant(self, instruction):
        """翻译浮点常量指令（由常量折叠产生）"""
        riscv_instructions = []
        result = instruction.result
        value = float(instruction.operands[0])
        data_type = self._get_data_type(instruction.types[0])
        
        # 获取目标寄存器
        dest_reg = self.allocator.allocate_register(result, data_type, True)
        
        # 加载浮点常量（简化实现）
        # 实际实现可能需要使用内存加载
        riscv_instructions.append(f"    # Load float constant {value}")
        riscv_instructions.append(f"    lui a0, {value.hex()}")
        riscv_instructions.append(f"    fmv.w.x {dest_reg}, a0")
        
        return riscv_instructions
    
    def _get_data_type(self, type_str):
        """将LLVM类型字符串转换为DataType枚举"""
        if type_str == 'i1':
            return DataType.I1
        elif type_str == 'i8':
            return DataType.I8
        elif type_str == 'i16':
            return DataType.I16
        elif type_str == 'i32':
            return DataType.I32
        elif type_str == 'i64':
            return DataType.I64
        elif type_str == 'float':
            return DataType.F32
        elif type_str == 'double':
            return DataType.F64
        return DataType.I32  # 默认
    
    def _translate_instruction(self, instruction):
        """翻译单条指令"""
        opcode = instruction.opcode
        
        # 返回指令
        if opcode == 'ret':
            return self._translate_ret(instruction)
        
        # 内存指令
        elif opcode in ['alloca', 'load', 'store']:
            return self._translate_memory(instruction)
        
        # 算术指令
        elif opcode in ['add', 'sub', 'mul', 'sdiv', 'and', 'or', 'xor', 
                        'fadd', 'fsub', 'fmul', 'fdiv']:
            return self._translate_arithmetic(instruction)
        
        # 移位指令
        elif opcode in ['shl', 'lshr', 'ashr']:
            return self._translate_shift(instruction)
        
        # 比较指令
        elif opcode in ['icmp', 'fcmp']:
            return self._translate_compare(instruction)
        
        # 分支指令
        elif opcode in ['br', 'jmp']:
            return self._translate_branch(instruction)
        
        # 函数调用
        elif opcode == 'call':
            return self._translate_call(instruction)
        
        # 类型转换
        elif opcode in ['trunc', 'zext', 'sext', 'fptrunc', 'fpext', 'fptoui', 'fptosi', 'uitofp', 'sitofp']:
            return self._translate_cast(instruction)
        
        # 未支持指令
        return [f"    # UNSUPPORTED: {instruction}"]
    
    def _translate_ret(self, instruction):
        """翻译返回指令"""
        riscv_instructions = []
        
        # 检查是否有返回值
        if not instruction.operands:
            # ret void - 无返回值
            riscv_instructions.append("    ret")
            return riscv_instructions
            
        ret_value = instruction.operands[0]
        ret_type = self._get_data_type(instruction.types[0])
        
        # 整数返回
        if ret_type in [DataType.I1, DataType.I8, DataType.I16, DataType.I32, DataType.I64]:
            if ret_value.isdigit() or (ret_value.startswith('-') and ret_value[1:].isdigit()):
                # 整数立即数
                riscv_instructions.append(f"    li a0, {ret_value}")
            elif ret_value.startswith('%'):
                # 从虚拟寄存器加载
                reg = self.allocator.get_physical_reg(ret_value)
                if reg:
                    # 检查是否在栈上
                    if '(sp)' in reg:
                        # 从栈加载到a0
                        riscv_instructions.append(f"    lw a0, {reg}")
                    else:
                        # 从寄存器移动到a0
                        riscv_instructions.append(f"    mv a0, {reg}")
                else:
                    # 如果寄存器映射不存在，直接使用立即数
                    riscv_instructions.append(f"    li a0, {ret_value}")
        # 浮点返回
        elif ret_type in [DataType.F32, DataType.F64]:
            if ret_value.replace('.', '').replace('-', '').replace('e', '').replace('E', '').replace('+', '').isdigit():
                # 浮点立即数需要特殊处理
                try:
                    float_val = float(ret_value)
                    import struct
                    # 将浮点数转换为32位整数表示
                    int_bits = struct.unpack('>I', struct.pack('>f', float_val))[0]
                    riscv_instructions.append(f"    li a0, 0x{int_bits:08x}")
                    riscv_instructions.append(f"    fmv.w.x fa0, a0")
                except ValueError:
                    # 如果转换失败，使用默认值
                    riscv_instructions.append(f"    li a0, 0")
                    riscv_instructions.append(f"    fmv.w.x fa0, a0")
            elif ret_value.startswith('%'):
                # 从虚拟寄存器加载
                reg = self.allocator.get_physical_reg(ret_value)
                if reg:
                    # 检查是否在栈上
                    if '(sp)' in reg:
                        # 从栈加载到fa0
                        load_instr = "flw" if ret_type == DataType.F32 else "fld"
                        riscv_instructions.append(f"    {load_instr} fa0, {reg}")
                    else:
                        # 从寄存器移动到fa0
                        riscv_instructions.append(f"    fmv.s fa0, {reg}")
        
        # 函数返回
        riscv_instructions.append("    ret")
        return riscv_instructions
    
    def _translate_memory(self, instruction):
        """翻译内存指令"""
        riscv_instructions = []
        opcode = instruction.opcode
        
        # 内存分配
        if opcode == 'alloca':
            result_reg = instruction.result
            data_type = self._get_data_type(instruction.types[0])
            size = data_type.value // 8  # 字节大小
            
            # 更新栈偏移
            self.allocator.stack_offset += size
            stack_offset = self.allocator.stack_offset
            
            # 将栈地址保存到寄存器
            dest_reg = self.allocator.allocate_register(result_reg, data_type)
            riscv_instructions.append(f"    # Allocate {size} bytes on stack")
            riscv_instructions.append(f"    addi {dest_reg}, sp, {stack_offset}")
        
        # 加载指令
        elif opcode == 'load':
            result_reg = instruction.result
            src_ptr = instruction.operands[0]
            data_type = self._get_data_type(instruction.types[0])
            
            # 根据数据类型确定是否为浮点
            is_float = data_type in [DataType.F32, DataType.F64]
            dest_reg_or_stack = self.allocator.allocate_register(result_reg, data_type, is_float)
            
            # 处理全局变量访问
            if src_ptr.startswith('@'):
                # 全局变量访问
                global_name = src_ptr[1:]  # 去掉@前缀
                riscv_instructions.append(f"    # Load from global variable {global_name}")
                
                # 如果目标是栈位置，使用临时寄存器
                if '(sp)' in dest_reg_or_stack:
                    temp_reg = self._get_temp_register()
                    riscv_instructions.append(f"    lui {temp_reg}, %hi({global_name})")
                    
                    # 根据数据类型选择正确的加载指令
                    if data_type == DataType.F32:
                        load_instr = "flw"
                    elif data_type == DataType.F64:
                        load_instr = "fld"
                    elif data_type == DataType.I32:
                        load_instr = "lw"
                    elif data_type == DataType.I16:
                        load_instr = "lh"
                    elif data_type == DataType.I8:
                        load_instr = "lb"
                    elif data_type == DataType.I64:
                        load_instr = "ld"
                    else:
                        load_instr = "lw"  # 默认整数加载
                    
                    riscv_instructions.append(f"    {load_instr} {temp_reg}, %lo({global_name})({temp_reg})")
                    self._store_to_stack_if_needed(temp_reg, dest_reg_or_stack, data_type, riscv_instructions)
                else:
                    riscv_instructions.append(f"    lui {dest_reg_or_stack}, %hi({global_name})")
                    
                    # 根据数据类型选择正确的加载指令
                    if data_type == DataType.F32:
                        load_instr = "flw"
                    elif data_type == DataType.F64:
                        load_instr = "fld"
                    elif data_type == DataType.I32:
                        load_instr = "lw"
                    elif data_type == DataType.I16:
                        load_instr = "lh"
                    elif data_type == DataType.I8:
                        load_instr = "lb"
                    elif data_type == DataType.I64:
                        load_instr = "ld"
                    else:
                        load_instr = "lw"  # 默认整数加载
                    
                    riscv_instructions.append(f"    {load_instr} {dest_reg_or_stack}, %lo({global_name})({dest_reg_or_stack})")
            else:
                # 局部变量访问
                src_reg = self.allocator.get_physical_reg(src_ptr)
                
                # 如果目标是栈位置，使用临时寄存器进行加载
                if '(sp)' in dest_reg_or_stack:
                    temp_reg = self._get_temp_register()
                    
                    # 根据数据类型选择正确的加载指令
                    if data_type == DataType.F32:
                        load_instr = "flw"
                    elif data_type == DataType.F64:
                        load_instr = "fld"
                    elif data_type == DataType.I32:
                        load_instr = "lw"
                    elif data_type == DataType.I16:
                        load_instr = "lh"
                    elif data_type == DataType.I8:
                        load_instr = "lb"
                    elif data_type == DataType.I64:
                        load_instr = "ld"
                    else:
                        load_instr = "lw"  # 默认整数加载
                    
                    riscv_instructions.append(f"    {load_instr} {temp_reg}, 0({src_reg})")
                    self._store_to_stack_if_needed(temp_reg, dest_reg_or_stack, data_type, riscv_instructions)
                else:
                    # 根据数据类型选择正确的加载指令
                    if data_type == DataType.F32:
                        load_instr = "flw"
                    elif data_type == DataType.F64:
                        load_instr = "fld"
                    elif data_type == DataType.I32:
                        load_instr = "lw"
                    elif data_type == DataType.I16:
                        load_instr = "lh"
                    elif data_type == DataType.I8:
                        load_instr = "lb"
                    elif data_type == DataType.I64:
                        load_instr = "ld"
                    else:
                        load_instr = "lw"  # 默认整数加载
                    
                    riscv_instructions.append(f"    {load_instr} {dest_reg_or_stack}, 0({src_reg})")
        
        # 存储指令
        elif opcode == 'store':
            value = instruction.operands[0]
            dest_ptr = instruction.operands[1]
            data_type = self._get_data_type(instruction.types[0])
            
            # 获取目标指针
            ptr_reg = self.allocator.get_physical_reg(dest_ptr)
            
            # 处理存储的值
            value_reg = None
            if value.isdigit() or (value.startswith('-') and value[1:].isdigit()):
                # 立即数，加载到临时寄存器
                temp_reg = self.allocator.allocate_register(f"%temp_{len(self.allocator.reg_map)}", data_type)
                riscv_instructions.append(f"    li {temp_reg}, {value}")
                value_reg = temp_reg
            elif value.startswith('%'):
                # 从虚拟寄存器获取
                value_reg_or_stack = self.allocator.get_physical_reg(value)
                
                # 如果值在栈上，需要先加载到寄存器
                if value_reg_or_stack and '(sp)' in value_reg_or_stack:
                    temp_reg = self._get_temp_register()
                    if data_type in [DataType.F32, DataType.F64]:
                        load_instr = "flw" if data_type == DataType.F32 else "fld"
                    else:
                        load_instr = "lw"
                    riscv_instructions.append(f"    {load_instr} {temp_reg}, {value_reg_or_stack}")
                    value_reg = temp_reg
                else:
                    value_reg = value_reg_or_stack
            
            # 根据数据类型选择存储指令
            store_instr = ""
            if data_type == DataType.I32:
                store_instr = "sw"
            elif data_type == DataType.I16:
                store_instr = "sh"
            elif data_type == DataType.I8:
                store_instr = "sb"
            elif data_type == DataType.F32:
                store_instr = "fsw"
            elif data_type == DataType.F64:
                store_instr = "fsd"
            else:
                store_instr = "sw"  # 默认
            
            if value_reg:
                riscv_instructions.append(f"    {store_instr} {value_reg}, 0({ptr_reg})")
        
        return riscv_instructions
    
    def _translate_arithmetic(self, instruction):
        """翻译算术指令"""
        riscv_instructions = []
        op = instruction.opcode
        result = instruction.result
        op1 = instruction.operands[0]
        op2 = instruction.operands[1]
        data_type = self._get_data_type(instruction.types[0])
        
        # 浮点运算
        if op in self.float_ops:
            is_float = True
            dest_reg_or_stack = self.allocator.allocate_register(result, data_type, is_float)
            
            # 获取操作数寄存器，处理栈溢出情况
            op1_reg = self._get_or_load_operand(op1, data_type, riscv_instructions)
            op2_reg = self._get_or_load_operand(op2, data_type, riscv_instructions)
            
            # 如果目标是栈位置，使用临时寄存器进行计算
            if '(sp)' in dest_reg_or_stack:
                temp_reg = self._get_temp_register()
                riscv_instructions.append(f"    {self.float_ops[op]} {temp_reg}, {op1_reg}, {op2_reg}")
                self._store_to_stack_if_needed(temp_reg, dest_reg_or_stack, data_type, riscv_instructions)
            else:
                riscv_instructions.append(f"    {self.float_ops[op]} {dest_reg_or_stack}, {op1_reg}, {op2_reg}")
            return riscv_instructions
        
        # 整数运算
        is_float = False
        dest_reg_or_stack = self.allocator.allocate_register(result, data_type, is_float)
        
        # 处理操作数1
        op1_reg = self._get_or_load_operand(op1, data_type, riscv_instructions)
        
        # 处理操作数2 - 特殊处理立即数优化
        if op2.isdigit() or (op2.startswith('-') and op2[1:].isdigit()):
            imm = int(op2)
            # 对于ADDI指令，立即数范围是-2048到2047
            if op in ['add', 'sub'] and -2048 <= imm <= 2047:
                if '(sp)' in dest_reg_or_stack:
                    temp_reg = self._get_temp_register()
                    if op == 'add':
                        riscv_instructions.append(f"    addi {temp_reg}, {op1_reg}, {imm}")
                    else:  # sub
                        riscv_instructions.append(f"    addi {temp_reg}, {op1_reg}, {-imm}")
                    self._store_to_stack_if_needed(temp_reg, dest_reg_or_stack, data_type, riscv_instructions)
                else:
                    if op == 'add':
                        riscv_instructions.append(f"    addi {dest_reg_or_stack}, {op1_reg}, {imm}")
                    else:  # sub
                        riscv_instructions.append(f"    addi {dest_reg_or_stack}, {op1_reg}, {-imm}")
                return riscv_instructions
        
        # 获取第二个操作数
        op2_reg = self._get_or_load_operand(op2, data_type, riscv_instructions)
        
        # 映射指令
        op_map = {
            'add': 'add',
            'sub': 'sub', 
            'mul': 'mul',
            'sdiv': 'div',
            'and': 'and',
            'or': 'or',
            'xor': 'xor'
        }
        
        if op in op_map:
            # 如果目标是栈位置，使用临时寄存器进行计算
            if '(sp)' in dest_reg_or_stack:
                temp_reg = self._get_temp_register()
                riscv_instructions.append(f"    {op_map[op]} {temp_reg}, {op1_reg}, {op2_reg}")
                self._store_to_stack_if_needed(temp_reg, dest_reg_or_stack, data_type, riscv_instructions)
            else:
                riscv_instructions.append(f"    {op_map[op]} {dest_reg_or_stack}, {op1_reg}, {op2_reg}")
        else:
            riscv_instructions.append(f"    # UNSUPPORTED ARITHMETIC: {op}")
        
        return riscv_instructions
    
    def _translate_shift(self, instruction):
        """翻译移位指令"""
        riscv_instructions = []
        op = instruction.opcode
        result = instruction.result
        op1 = instruction.operands[0]
        op2 = instruction.operands[1]
        data_type = self._get_data_type(instruction.types[0])
        
        dest_reg = self.allocator.allocate_register(result, data_type)
        
        # 处理操作数1
        if op1.startswith('%'):
            op1_reg = self.allocator.get_physical_reg(op1)
        else:
            op1_reg = op1
        
        # 处理操作数2
        if op2.isdigit() or (op2.startswith('-') and op2[1:].isdigit()):
            imm = int(op2)
            # 映射指令
            op_map = {
                'shl': 'slli',
                'lshr': 'srli',
                'ashr': 'srai'
            }
            
            if op in op_map and 0 <= imm <= 31:
                riscv_instructions.append(f"    {op_map[op]} {dest_reg}, {op1_reg}, {imm}")
                return riscv_instructions
            else:
                # 加载到临时寄存器
                op2_reg = self.allocator.allocate_register(f"%temp2_{len(self.allocator.reg_map)}", data_type)
                riscv_instructions.append(f"    li {op2_reg}, {op2}")
        elif op2.startswith('%'):
            op2_reg = self.allocator.get_physical_reg(op2)
        else:
            op2_reg = op2
        
        # 非立即数移位
        op_map = {
            'shl': 'sll',
            'lshr': 'srl',
            'ashr': 'sra'
        }
        
        if op in op_map:
            riscv_instructions.append(f"    {op_map[op]} {dest_reg}, {op1_reg}, {op2_reg}")
        else:
            riscv_instructions.append(f"    # UNSUPPORTED SHIFT: {op}")
        
        return riscv_instructions
    
    def _translate_compare(self, instruction):
        """翻译比较指令"""
        riscv_instructions = []
        cmp_type = instruction.types[1] if instruction.opcode == 'icmp' else instruction.types[0]
        result = instruction.result
        op1 = instruction.operands[0]
        op2 = instruction.operands[1]
        is_float = instruction.opcode == 'fcmp'
        
        # 获取目标寄存器
        data_type = DataType.I32  # 比较结果是布尔值
        dest_reg = self.allocator.allocate_register(result, data_type, is_float)
        
        # 处理操作数1
        op1_reg = self.allocator.get_physical_reg(op1) if op1.startswith('%') else op1
        
        # 处理操作数2
        op2_reg = None
        if op2.startswith('%'):
            op2_reg = self.allocator.get_physical_reg(op2)
        elif op2.isdigit() or (op2.startswith('-') and op2[1:].isdigit()):
            # 整数比较
            if not is_float:
                imm = int(op2)
                if -2048 <= imm <= 2047:
                    op2_reg = imm
                else:
                    temp_reg = self.allocator.allocate_register(f"%temp_{len(self.allocator.reg_map)}", DataType.I32)
                    riscv_instructions.append(f"    li {temp_reg}, {op2}")
                    op2_reg = temp_reg
            else:
                # 浮点比较
                temp_reg = self.allocator.allocate_register(f"%temp_{len(self.allocator.reg_map)}", DataType.F32, True)
                riscv_instructions.append(f"    lui a0, {float(op2).hex()}")
                riscv_instructions.append(f"    fmv.w.x {temp_reg}, a0")
                op2_reg = temp_reg
        
        # 浮点比较
        if is_float:
            cmp_map = {
                'oeq': 'feq.s',
                'ogt': 'fgt.s',
                'oge': 'fge.s',
                'olt': 'flt.s',
                'ole': 'fle.s',
                'one': 'fne.s'
            }
            
            if cmp_type in cmp_map:
                riscv_instructions.append(f"    {cmp_map[cmp_type]} {dest_reg}, {op1_reg}, {op2_reg}")
            else:
                riscv_instructions.append(f"    # UNSUPPORTED FLOAT CMP: {cmp_type}")
            return riscv_instructions
        
        # 整数比较
        cmp_map = {
            'eq': 'seqz',  # 使用序列优化
            'ne': 'snez',
            'slt': 'slt',
            'sge': 'sgt',  # a >= b 等价于 b < a
            'sgt': 'sgt',
            'sle': 'slt',  # a <= b 等价于 b > a
            'ult': 'sltu',
            'uge': 'sgtu',
            'ugt': 'sgtu',
            'ule': 'sltu'
        }
        
        if cmp_type in cmp_map:
            # 使用RISC-V的set指令优化
            if cmp_type in ['eq', 'ne']:
                riscv_instructions.append(f"    xor {dest_reg}, {op1_reg}, {op2_reg}")
                riscv_instructions.append(f"    {cmp_map[cmp_type]} {dest_reg}, {dest_reg}")
            elif cmp_type in ['sge', 'sle']:
                # 优化：a >= b 等价于 b <= a
                riscv_instructions.append(f"    {cmp_map[cmp_type]} {dest_reg}, {op2_reg}, {op1_reg}")
            else:
                riscv_instructions.append(f"    {cmp_map[cmp_type]} {dest_reg}, {op1_reg}, {op2_reg}")
        else:
            riscv_instructions.append(f"    # UNSUPPORTED CMP TYPE: {cmp_type}")
        
        return riscv_instructions
    
    def _translate_branch(self, instruction):
        """翻译分支指令"""
        riscv_instructions = []
        
        if instruction.opcode == 'br' and len(instruction.operands) == 3:
            # 条件分支
            cond = instruction.operands[0]
            true_label = instruction.operands[1]
            false_label = instruction.operands[2]
            
            # 获取条件寄存器
            cond_reg = self.allocator.get_physical_reg(cond)
            
            # 映射标签
            true_target = self.label_map.get(true_label, true_label)
            false_target = self.label_map.get(false_label, false_label)
            
            # 生成分支指令
            riscv_instructions.append(f"    bnez {cond_reg}, {true_target}")
            riscv_instructions.append(f"    j {false_target}")
            
        elif instruction.opcode in ['br', 'jmp'] and len(instruction.operands) == 1:
            # 无条件跳转
            target = instruction.operands[0]
            target_label = self.label_map.get(target, target)
            riscv_instructions.append(f"    j {target_label}")
        
        return riscv_instructions
    
    def _translate_call(self, instruction):
        """翻译函数调用指令"""
        riscv_instructions = []
        func_name = instruction.operands[0]
        args = instruction.operands[1:]
        result = instruction.result
        
        # 保存调用者保存的寄存器
        riscv_instructions.append("    # Save caller-saved registers")
        for reg in self.allocator.temp_regs + self.allocator.float_regs:
            if self.allocator.reg_in_use.get(reg, False):
                temp_offset = self.allocator.stack_offset
                self.allocator.stack_offset += 4
                riscv_instructions.append(f"    sw {reg}, {temp_offset}(sp)")
        
        # 设置参数
        for i, arg in enumerate(args):
            if i < len(self.allocator.param_regs):
                param_reg = self.allocator.param_regs[i]
                if arg.startswith('%'):
                    arg_reg = self.allocator.get_physical_reg(arg)
                    riscv_instructions.append(f"    mv {param_reg}, {arg_reg}")
                else:
                    riscv_instructions.append(f"    li {param_reg}, {arg}")
        
        # 函数调用
        riscv_instructions.append(f"    call {func_name}")
        
        # 恢复调用者保存的寄存器
        riscv_instructions.append("    # Restore caller-saved registers")
        for reg in reversed(self.allocator.temp_regs + self.allocator.float_regs):
            if self.allocator.reg_in_use.get(reg, False):
                temp_offset = self.allocator.stack_offset - 4
                self.allocator.stack_offset -= 4
                riscv_instructions.append(f"    lw {reg}, {temp_offset}(sp)")
        
        # 处理返回值
        if result:
            data_type = self._get_data_type(instruction.types[0])
            is_float = data_type in [DataType.F32, DataType.F64]
            dest_reg = self.allocator.allocate_register(result, data_type, is_float)
            
            if is_float:
                riscv_instructions.append(f"    fmv.s {dest_reg}, fa0")
            else:
                riscv_instructions.append(f"    mv {dest_reg}, a0")
        
        return riscv_instructions
    
    def _translate_cast(self, instruction):
        """翻译类型转换指令"""
        riscv_instructions = []
        opcode = instruction.opcode
        result = instruction.result
        value = instruction.operands[0]
        
        # 获取源和目标类型
        src_type = self._get_data_type(instruction.types[0])
        dest_type = self._get_data_type(instruction.types[1]) if len(instruction.types) > 1 else src_type
        
        # 获取目标寄存器
        is_float_dest = dest_type in [DataType.F32, DataType.F64]
        dest_reg = self.allocator.allocate_register(result, dest_type, is_float_dest)
        
        # 获取源寄存器
        is_float_src = src_type in [DataType.F32, DataType.F64]
        value_reg = self.allocator.get_physical_reg(value) if value.startswith('%') else value
        
        # 整数到浮点
        if opcode in ['sitofp', 'uitofp']:
            if opcode == 'sitofp':
                riscv_instructions.append(f"    fcvt.s.w {dest_reg}, {value_reg}")
            else:
                riscv_instructions.append(f"    fcvt.s.wu {dest_reg}, {value_reg}")
        
        # 浮点到整数
        elif opcode in ['fptosi', 'fptoui']:
            if opcode == 'fptosi':
                riscv_instructions.append(f"    fcvt.w.s {dest_reg}, {value_reg}, rtz")
            else:
                riscv_instructions.append(f"    fcvt.wu.s {dest_reg}, {value_reg}, rtz")
        
        # 浮点精度转换
        elif opcode == 'fptrunc':
            riscv_instructions.append(f"    fcvt.s.d {dest_reg}, {value_reg}")
        elif opcode == 'fpext':
            riscv_instructions.append(f"    fcvt.d.s {dest_reg}, {value_reg}")
        
        # 整数截断
        elif opcode == 'trunc':
            # 通常只需移动寄存器，因为RISC-V寄存器是32位
            riscv_instructions.append(f"    mv {dest_reg}, {value_reg}")
        
        # 整数扩展
        elif opcode in ['zext', 'sext']:
            if opcode == 'zext':
                riscv_instructions.append(f"    andi {dest_reg}, {value_reg}, 0xFFFF")  # 16位示例
            else:
                riscv_instructions.append(f"    slli {dest_reg}, {value_reg}, 16")
                riscv_instructions.append(f"    srai {dest_reg}, {dest_reg}, 16")
        
        return riscv_instructions
    
    def _extract_global_variables(self, ir_code):
        """从LLVM IR代码中提取全局变量声明"""
        global_vars = []
        lines = ir_code.strip().split('\n')
        
        for line in lines:
            line = line.strip()
            # 匹配全局变量声明: @g_b = dso_local global i32 3
            global_match = re.match(r'@(\w+)\s*=\s*(?:dso_local\s+)?global\s+(\w+)\s+(.+)', line)
            if global_match:
                var_name = global_match.group(1)
                var_type = global_match.group(2)
                var_value = global_match.group(3)
                global_vars.append((var_name, var_type, var_value))
        
        return global_vars

    def _get_or_load_operand(self, operand, data_type, riscv_instructions):
        """获取操作数对应的寄存器，如果操作数在栈上则先加载到临时寄存器"""
        if operand.startswith('%'):
            # 虚拟寄存器
            reg = self.allocator.get_physical_reg(operand)
            if reg and '(sp)' in reg:
                # 变量在栈上，需要加载到临时寄存器
                temp_reg = self._get_temp_register()
                if data_type in [DataType.F32, DataType.F64]:
                    load_instr = "flw" if data_type == DataType.F32 else "fld"
                else:
                    load_instr = "lw"
                riscv_instructions.append(f"    {load_instr} {temp_reg}, {reg}")
                return temp_reg
            return reg
        elif operand.isdigit() or (operand.startswith('-') and operand[1:].isdigit()):
            # 立即数，加载到临时寄存器
            temp_reg = self._get_temp_register()
            riscv_instructions.append(f"    li {temp_reg}, {operand}")
            return temp_reg
        else:
            return operand
    
    def _get_temp_register(self):
        """获取一个临时寄存器"""
        # 改进的临时寄存器分配，避免重复使用
        temp_candidates = ['s0', 's1', 's2', 't6']
        for reg in temp_candidates:
            if not self.allocator.reg_in_use.get(reg, False):
                # 临时标记为使用中，避免重复分配
                self.allocator.reg_in_use[reg] = True
                return reg
        return 's0'  # 如果都被使用，使用s0作为备用
    
    def _get_unique_temp_registers(self, count):
        """获取多个不同的临时寄存器"""
        temp_candidates = ['s0', 's1', 's2', 's3', 't6']
        allocated_regs = []
        
        for i in range(min(count, len(temp_candidates))):
            reg = temp_candidates[i]
            if not self.allocator.reg_in_use.get(reg, False):
                self.allocator.reg_in_use[reg] = True
                allocated_regs.append(reg)
            else:
                # 如果寄存器被占用，使用备用方案
                allocated_regs.append(f's{i}')
        
        # 如果需要的寄存器数量超过可用数量，重复使用
        while len(allocated_regs) < count:
            allocated_regs.append(allocated_regs[len(allocated_regs) % len(temp_candidates)])
        
        return allocated_regs
    
    def _store_to_stack_if_needed(self, result_reg, stack_location, data_type, riscv_instructions):
        """如果目标是栈位置，将寄存器值存储到栈上"""
        if '(sp)' in stack_location:
            if data_type in [DataType.F32, DataType.F64]:
                store_instr = "fsw" if data_type == DataType.F32 else "fsd"
            else:
                store_instr = "sw"
            riscv_instructions.append(f"    {store_instr} {result_reg}, {stack_location}")
            return True
        return False

# 使用示例
if __name__ == "__main__":
    # 示例LLVM IR代码
    sample_ir = """
define dso_local i32 @main() {
entry:
    %a = alloca i32
    store i32 5, i32* %a
    %b = load i32, i32* %a
    %sum = add i32 %b, 3
    %cmp = icmp eq i32 %sum, 8
    br i1 %cmp, label %true_block, label %false_block
    
true_block:
    %f = sitofp i32 %sum to float
    %f2 = fadd float %f, 1.5
    ret i32 1
    
false_block:
    ret i32 0
}
"""

    translator = OptimizedLLVMIRTranslator()
    riscv_code = translator.translate(sample_ir)
    
    print("Generated RISC-V Assembly:")
    print(riscv_code)