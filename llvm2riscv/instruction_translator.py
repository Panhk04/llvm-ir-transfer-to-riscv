"""
LLVM IR指令翻译器模块
"""

import re
import struct
from types_and_constants import (
    DataType, get_data_type, calculate_type_size,
    FLOAT_OPS_MAP, INT_OPS_MAP, SHIFT_OPS_MAP, 
    CMP_OPS_MAP, FLOAT_CMP_MAP
)

class InstructionTranslator:
    def __init__(self, allocator):
        self.allocator = allocator
        self.label_map = {}
    
    def set_label_map(self, label_map):
        """设置标签映射"""
        self.label_map = label_map
    
    def translate_instruction(self, instruction):
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
        
        # 地址计算指令
        elif opcode == 'getelementptr':
            return self._translate_getelementptr(instruction)
        
        # 常量指令（优化产生的）
        elif opcode == 'const':
            return self._translate_constant(instruction)
        elif opcode == 'fconst':
            return self._translate_fconstant(instruction)
        
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
        ret_type = get_data_type(instruction.types[0])
        
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
        
        # 内存分配 - 在预处理阶段已处理，这里跳过
        if opcode == 'alloca':
            return []  # 已在预处理中处理
        
        # 加载指令
        elif opcode == 'load':
            result_reg = instruction.result
            src_ptr = instruction.operands[0]
            data_type = get_data_type(instruction.types[0])
            
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
                    temp_reg = self.allocator.get_temp_register()
                    riscv_instructions.append(f"    lui {temp_reg}, %hi({global_name})")
                    
                    # 根据数据类型选择正确的加载指令
                    load_instr = self._get_load_instruction(data_type)
                    
                    riscv_instructions.append(f"    {load_instr} {temp_reg}, %lo({global_name})({temp_reg})")
                    riscv_instructions.append(f"    sw {temp_reg}, {dest_reg_or_stack}")
                else:
                    riscv_instructions.append(f"    lui {dest_reg_or_stack}, %hi({global_name})")
                    
                    # 根据数据类型选择正确的加载指令
                    load_instr = self._get_load_instruction(data_type)
                    
                    riscv_instructions.append(f"    {load_instr} {dest_reg_or_stack}, %lo({global_name})({dest_reg_or_stack})")
            else:
                # 局部变量访问
                src_reg_or_stack = self.allocator.get_physical_reg(src_ptr)
                
                if not src_reg_or_stack:
                    riscv_instructions.append(f"    # ERROR: Cannot resolve pointer {src_ptr}")
                    return riscv_instructions
                
                # 获取源地址寄存器（如果在栈上，先加载到寄存器）
                if '(sp)' in src_reg_or_stack:
                    addr_reg = self.allocator.get_temp_register()
                    riscv_instructions.append(f"    lw {addr_reg}, {src_reg_or_stack}")
                else:
                    addr_reg = src_reg_or_stack
                
                # 如果目标是栈位置，使用临时寄存器进行加载
                if '(sp)' in dest_reg_or_stack:
                    temp_reg = self.allocator.get_temp_register()
                    
                    # 根据数据类型选择正确的加载指令
                    load_instr = self._get_load_instruction(data_type)
                    
                    riscv_instructions.append(f"    {load_instr} {temp_reg}, 0({addr_reg})")
                    riscv_instructions.append(f"    sw {temp_reg}, {dest_reg_or_stack}")
                else:
                    # 根据数据类型选择正确的加载指令
                    load_instr = self._get_load_instruction(data_type)
                    
                    riscv_instructions.append(f"    {load_instr} {dest_reg_or_stack}, 0({addr_reg})")
        
        # 存储指令
        elif opcode == 'store':
            value = instruction.operands[0]
            dest_ptr = instruction.operands[1]
            data_type = get_data_type(instruction.types[0])
            
            # 获取目标指针
            ptr_reg_or_stack = self.allocator.get_physical_reg(dest_ptr)
            
            if not ptr_reg_or_stack:
                riscv_instructions.append(f"    # ERROR: Cannot resolve destination pointer {dest_ptr}")
                return riscv_instructions
            
            # 获取目标地址寄存器
            if '(sp)' in ptr_reg_or_stack:
                ptr_reg = self.allocator.get_temp_register()
                riscv_instructions.append(f"    lw {ptr_reg}, {ptr_reg_or_stack}")
            else:
                ptr_reg = ptr_reg_or_stack
            
            # 处理存储的值
            value_reg = self._get_or_load_operand(value, data_type, riscv_instructions)
            
            # 根据数据类型选择存储指令
            store_instr = self._get_store_instruction(data_type)
            
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
        data_type = get_data_type(instruction.types[0])
        
        # 浮点运算
        if op in FLOAT_OPS_MAP:
            is_float = True
            dest_reg_or_stack = self.allocator.allocate_register(result, data_type, is_float)
            
            # 获取操作数寄存器，处理栈溢出情况
            op1_reg = self._get_or_load_operand(op1, data_type, riscv_instructions)
            op2_reg = self._get_or_load_operand(op2, data_type, riscv_instructions)
            
            # 如果目标是栈位置，使用临时寄存器进行计算
            if '(sp)' in dest_reg_or_stack:
                temp_reg = self.allocator.get_temp_register()
                riscv_instructions.append(f"    {FLOAT_OPS_MAP[op]} {temp_reg}, {op1_reg}, {op2_reg}")
                self.allocator.store_to_stack_if_needed(temp_reg, dest_reg_or_stack, data_type, riscv_instructions)
            else:
                riscv_instructions.append(f"    {FLOAT_OPS_MAP[op]} {dest_reg_or_stack}, {op1_reg}, {op2_reg}")
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
                    temp_reg = self.allocator.get_temp_register()
                    if op == 'add':
                        riscv_instructions.append(f"    addi {temp_reg}, {op1_reg}, {imm}")
                    else:  # sub
                        riscv_instructions.append(f"    addi {temp_reg}, {op1_reg}, {-imm}")
                    self.allocator.store_to_stack_if_needed(temp_reg, dest_reg_or_stack, data_type, riscv_instructions)
                else:
                    if op == 'add':
                        riscv_instructions.append(f"    addi {dest_reg_or_stack}, {op1_reg}, {imm}")
                    else:  # sub
                        riscv_instructions.append(f"    addi {dest_reg_or_stack}, {op1_reg}, {-imm}")
                return riscv_instructions
        
        # 获取第二个操作数
        op2_reg = self._get_or_load_operand(op2, data_type, riscv_instructions)
        
        if op in INT_OPS_MAP:
            # 如果目标是栈位置，使用临时寄存器进行计算
            if '(sp)' in dest_reg_or_stack:
                temp_reg = self.allocator.get_temp_register()
                riscv_instructions.append(f"    {INT_OPS_MAP[op]} {temp_reg}, {op1_reg}, {op2_reg}")
                self.allocator.store_to_stack_if_needed(temp_reg, dest_reg_or_stack, data_type, riscv_instructions)
            else:
                riscv_instructions.append(f"    {INT_OPS_MAP[op]} {dest_reg_or_stack}, {op1_reg}, {op2_reg}")
        else:
            riscv_instructions.append(f"    # UNSUPPORTED ARITHMETIC: {op}")
        
        return riscv_instructions
    
    def _translate_getelementptr(self, instruction):
        """翻译getelementptr指令 - 计算数组元素地址"""
        riscv_instructions = []
        result = instruction.result
        base_ptr = instruction.operands[0]  # 基础指针
        indices = instruction.operands[1:]  # 索引列表
        
        # 获取目标寄存器
        dest_reg_or_stack = self.allocator.allocate_register(result, DataType.I32, False)
        
        # 获取基础指针 - 关键修复：正确处理alloca分配的指针
        base_reg = None
        if base_ptr.startswith('%'):
            # 首先检查是否在stack_frame中（alloca分配的变量）
            if base_ptr in self.allocator.stack_frame:
                # 直接获取栈偏移并计算地址
                stack_offset = self.allocator.stack_frame[base_ptr]
                temp_reg = self.allocator.get_temp_register()
                riscv_instructions.append(f"    # Calculate address for {base_ptr} at stack offset {stack_offset}")
                riscv_instructions.append(f"    addi {temp_reg}, sp, {stack_offset}")
                base_reg = temp_reg
            else:
                # 尝试从寄存器映射获取
                base_reg_or_stack = self.allocator.get_physical_reg(base_ptr)
                if base_reg_or_stack:
                    if '(sp)' in base_reg_or_stack:
                        # 基础指针在栈上，先加载到临时寄存器
                        temp_reg = self.allocator.get_temp_register()
                        riscv_instructions.append(f"    lw {temp_reg}, {base_reg_or_stack}")
                        base_reg = temp_reg
                    else:
                        base_reg = base_reg_or_stack
                else:
                    # 如果既不在stack_frame中也不在reg_map中，这可能是一个错误
                    # 但我们可以尝试强制为它分配一个栈位置
                    riscv_instructions.append(f"    # WARNING: Base pointer {base_ptr} not found, creating default mapping")
                    # 为这个指针创建一个栈位置
                    if base_ptr not in self.allocator.stack_frame:
                        self.allocator.stack_offset += 32  # 默认为数组分配32字节
                        self.allocator.stack_frame[base_ptr] = self.allocator.stack_offset
                    
                    stack_offset = self.allocator.stack_frame[base_ptr]
                    temp_reg = self.allocator.get_temp_register()
                    riscv_instructions.append(f"    addi {temp_reg}, sp, {stack_offset}")
                    base_reg = temp_reg
        else:
            base_reg = base_ptr
        
        # 确保base_reg不为空
        if not base_reg:
            riscv_instructions.append(f"    # ERROR: Failed to resolve base pointer {base_ptr}")
            return riscv_instructions
        
        # 计算地址偏移
        total_offset = 0
        element_size = 4  # i32 = 4字节
        
        if len(indices) > 0:
            # 跳过第一个索引（通常是0，表示指向数组开始）
            start_idx = 1 if len(indices) > 1 and indices[0] == '0' else 0
            
            # 处理剩余的索引
            for i in range(start_idx, len(indices)):
                idx = indices[i]
                if idx.isdigit():
                    # 对于多维数组 [4 x [2 x i32]]，每个内层数组有2个元素
                    if i == start_idx:
                        # 第一维索引，每个元素是一个[2 x i32]数组
                        inner_array_size = 2 * element_size  # 2个i32元素
                        total_offset += int(idx) * inner_array_size
                    else:
                        # 第二维索引，直接是元素偏移
                        total_offset += int(idx) * element_size
        
        # 生成最终地址计算指令
        if total_offset == 0:
            # 偏移为0，直接复制基址
            if '(sp)' in dest_reg_or_stack:
                temp_reg = self.allocator.get_temp_register()
                riscv_instructions.append(f"    mv {temp_reg}, {base_reg}")
                self.allocator.store_to_stack_if_needed(temp_reg, dest_reg_or_stack, DataType.I32, riscv_instructions)
            else:
                riscv_instructions.append(f"    mv {dest_reg_or_stack}, {base_reg}")
        else:
            # 添加偏移
            if '(sp)' in dest_reg_or_stack:
                temp_reg = self.allocator.get_temp_register()
                riscv_instructions.append(f"    addi {temp_reg}, {base_reg}, {total_offset}")
                self.allocator.store_to_stack_if_needed(temp_reg, dest_reg_or_stack, DataType.I32, riscv_instructions)
            else:
                riscv_instructions.append(f"    addi {dest_reg_or_stack}, {base_reg}, {total_offset}")
        
        return riscv_instructions
    
    def _translate_constant(self, instruction):
        """翻译常量指令（由常量折叠产生）"""
        riscv_instructions = []
        result = instruction.result
        value = instruction.operands[0]
        data_type = get_data_type(instruction.types[0])
        
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
        data_type = get_data_type(instruction.types[0])
        
        # 获取目标寄存器
        dest_reg = self.allocator.allocate_register(result, data_type, True)
        
        # 加载浮点常量（简化实现）
        # 实际实现可能需要使用内存加载
        riscv_instructions.append(f"    # Load float constant {value}")
        riscv_instructions.append(f"    lui a0, {value.hex()}")
        riscv_instructions.append(f"    fmv.w.x {dest_reg}, a0")
        
        return riscv_instructions
    
    # 简化版本的其他翻译方法（为了节省空间，只保留核心逻辑）
    def _translate_shift(self, instruction):
        """翻译移位指令"""
        return [f"    # SHIFT: {instruction.opcode}"]
    
    def _translate_compare(self, instruction):
        """翻译比较指令"""
        return [f"    # COMPARE: {instruction.opcode}"]
    
    def _translate_branch(self, instruction):
        """翻译分支指令"""
        return [f"    # BRANCH: {instruction.opcode}"]
    
    def _translate_call(self, instruction):
        """翻译函数调用指令"""
        return [f"    # CALL: {instruction.operands[0]}"]
    
    def _translate_cast(self, instruction):
        """翻译类型转换指令"""
        return [f"    # CAST: {instruction.opcode}"]
    
    # 辅助方法
    def _get_load_instruction(self, data_type):
        """根据数据类型获取加载指令"""
        if data_type == DataType.F32:
            return "flw"
        elif data_type == DataType.F64:
            return "fld"
        elif data_type == DataType.I32:
            return "lw"
        elif data_type == DataType.I16:
            return "lh"
        elif data_type == DataType.I8:
            return "lb"
        elif data_type == DataType.I64:
            return "ld"
        else:
            return "lw"  # 默认整数加载
    
    def _get_store_instruction(self, data_type):
        """根据数据类型获取存储指令"""
        if data_type == DataType.I32:
            return "sw"
        elif data_type == DataType.I16:
            return "sh"
        elif data_type == DataType.I8:
            return "sb"
        elif data_type == DataType.F32:
            return "fsw"
        elif data_type == DataType.F64:
            return "fsd"
        else:
            return "sw"  # 默认
    
    def _get_or_load_operand(self, operand, data_type, riscv_instructions):
        """获取操作数对应的寄存器，如果操作数在栈上则先加载到临时寄存器"""
        if operand.startswith('%'):
            # 虚拟寄存器
            reg = self.allocator.get_physical_reg(operand)
            if reg and '(sp)' in reg:
                # 变量在栈上，需要加载到临时寄存器
                temp_reg = self.allocator.get_temp_register()
                if data_type in [DataType.F32, DataType.F64]:
                    load_instr = "flw" if data_type == DataType.F32 else "fld"
                else:
                    load_instr = "lw"
                riscv_instructions.append(f"    {load_instr} {temp_reg}, {reg}")
                return temp_reg
            return reg
        elif operand.isdigit() or (operand.startswith('-') and operand[1:].isdigit()):
            # 立即数，加载到临时寄存器
            temp_reg = self.allocator.get_temp_register()
            riscv_instructions.append(f"    li {temp_reg}, {operand}")
            return temp_reg
        else:
            return operand