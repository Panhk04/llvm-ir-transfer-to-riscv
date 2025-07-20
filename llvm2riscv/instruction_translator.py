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
        self.stack_size = 576  # 默认栈大小，将在函数翻译时更新
    
    def set_label_map(self, label_map):
        """设置标签映射"""
        self.label_map = label_map
    
    def set_stack_size(self, stack_size):
        """设置当前函数的栈大小"""
        self.stack_size = stack_size
    
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
            riscv_instructions.append("    # Function epilogue")
            riscv_instructions.append(f"    lw ra, {self.stack_size-4}(sp)")  # 使用动态栈大小
            riscv_instructions.append(f"    lw s0, {self.stack_size-8}(sp)")  # 使用动态栈大小
            riscv_instructions.append(f"    addi sp, sp, {self.stack_size}")  # 使用动态栈大小
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
        
        # 函数尾声 - 使用动态栈大小
        riscv_instructions.append("    # Function epilogue")
        riscv_instructions.append(f"    lw ra, {self.stack_size-4}(sp)")  # 使用动态栈大小
        riscv_instructions.append(f"    lw s0, {self.stack_size-8}(sp)")  # 使用动态栈大小
        riscv_instructions.append(f"    addi sp, sp, {self.stack_size}")  # 使用动态栈大小
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
            # 处理alloca分配的变量访问
            elif src_ptr in self.allocator.stack_frame:
                # 直接从alloca分配的栈位置加载
                stack_location = f"{self.allocator.stack_frame[src_ptr]}(sp)"
                load_instr = self._get_load_instruction(data_type)
                
                if '(sp)' in dest_reg_or_stack:
                    temp_reg = self.allocator.get_temp_register()
                    riscv_instructions.append(f"    {load_instr} {temp_reg}, {stack_location}")
                    store_instr = self._get_store_instruction(data_type)
                    riscv_instructions.append(f"    {store_instr} {temp_reg}, {dest_reg_or_stack}")
                else:
                    riscv_instructions.append(f"    {load_instr} {dest_reg_or_stack}, {stack_location}")
            else:
                # 处理其他类型的指针（如通过getelementptr计算的地址）
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
            
            # 处理存储的值
            value_reg = self._get_or_load_operand(value, data_type, riscv_instructions)
            
            # 处理目标指针 - 修复alloca变量的存储逻辑
            if dest_ptr in self.allocator.stack_frame:
                # 直接存储到alloca分配的栈位置
                stack_location = f"{self.allocator.stack_frame[dest_ptr]}(sp)"
                store_instr = self._get_store_instruction(data_type)
                if value_reg:
                    riscv_instructions.append(f"    {store_instr} {value_reg}, {stack_location}")
            else:
                # 处理其他类型的指针（如通过getelementptr计算的地址）
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
        """翻译getelementptr指令 - 修复数组维度解析和步长计算"""
        riscv_instructions = []
        result = instruction.result
        base_ptr = instruction.operands[0]
        indices = instruction.operands[1:]
        
        dest_reg_or_stack = self.allocator.allocate_register(result, DataType.I32, False)

        # 1. 获取基础指针地址
        base_reg = None
        if base_ptr in self.allocator.stack_frame:
            stack_offset = self.allocator.stack_frame[base_ptr]
            temp_reg = self.allocator.get_temp_register()
            riscv_instructions.append(f"    addi {temp_reg}, sp, {stack_offset}")
            base_reg = temp_reg
        else:
            base_reg_or_stack = self.allocator.get_physical_reg(base_ptr)
            if base_reg_or_stack:
                if '(sp)' in base_reg_or_stack:
                    temp_reg = self.allocator.get_temp_register()
                    riscv_instructions.append(f"    lw {temp_reg}, {base_reg_or_stack}")
                    base_reg = temp_reg
                else:
                    base_reg = base_reg_or_stack
        
        if not base_reg:
            riscv_instructions.append(f"    # ERROR: Failed to resolve base pointer {base_ptr}")
            return riscv_instructions

        # 2. 解析数组类型和维度 - 修复递归解析
        array_type_str = instruction.types[0]
        dims = []
        current_str = array_type_str
        
        # 递归提取所有维度
        while '[' in current_str and 'x' in current_str:
            start_idx = current_str.find('[')
            end_idx = current_str.find(']', start_idx)
            if end_idx == -1:
                break
                
            dim_part = current_str[start_idx+1:end_idx]
            parts = dim_part.split('x', 1)
            
            if not parts:
                break
                
            try:
                dim_size = int(parts[0].strip())
                dims.append(dim_size)
            except ValueError:
                break
                
            if len(parts) > 1:
                current_str = parts[1].strip()
            else:
                break
        
        # 获取最终元素类型
        element_type_str = current_str.replace(']', '').replace('*', '').strip()
        element_size = calculate_type_size(element_type_str)  # 直接传递字符串，不转换为DataType

        # 3. 计算步长 - 修复步长计算逻辑
        strides = []
        current_stride = element_size
        # 从最内层到最外层计算步长
        for dim in reversed(dims):
            strides.insert(0, current_stride)
            current_stride *= dim

        # 跳过第一个索引（通常是0，指向数组本身）
        indices_to_process = indices[1:]
        
        # 4. 计算偏移量
        offset_reg = self.allocator.get_temp_register()
        riscv_instructions.append(f"    li {offset_reg}, 0")
        
        # 防止索引数量超过维度数量
        if len(indices_to_process) > len(strides):
            riscv_instructions.append("    # WARNING: More indices than dimensions")
            # 只处理有效维度
            indices_to_process = indices_to_process[:len(strides)]
        
        for i, index_val in enumerate(indices_to_process):
            stride = strides[i]
            
            index_reg = self._get_or_load_operand(index_val, DataType.I32, riscv_instructions)
            
            temp_mul_reg = self.allocator.get_temp_register()
            stride_reg = self.allocator.get_temp_register()
            
            riscv_instructions.append(f"    li {stride_reg}, {stride}")
            riscv_instructions.append(f"    mul {temp_mul_reg}, {index_reg}, {stride_reg}")
            riscv_instructions.append(f"    add {offset_reg}, {offset_reg}, {temp_mul_reg}")

        # 5. 计算最终地址
        final_addr_reg = self.allocator.get_temp_register()
        riscv_instructions.append(f"    add {final_addr_reg}, {base_reg}, {offset_reg}")

        # 6. 存储最终地址
        if '(sp)' in dest_reg_or_stack:
            riscv_instructions.append(f"    sw {final_addr_reg}, {dest_reg_or_stack}")
        else:
            riscv_instructions.append(f"    mv {dest_reg_or_stack}, {final_addr_reg}")
            
        return riscv_instructions
                    
    def _translate_shift(self, instruction):
        """翻译移位指令"""
        riscv_instructions = []
        op = instruction.opcode
        result = instruction.result
        op1 = instruction.operands[0]
        op2 = instruction.operands[1]
        data_type = get_data_type(instruction.types[0])
        
        dest_reg_or_stack = self.allocator.allocate_register(result, data_type, False)
        op1_reg = self._get_or_load_operand(op1, data_type, riscv_instructions)
        op2_reg = self._get_or_load_operand(op2, data_type, riscv_instructions)
        
        if op in SHIFT_OPS_MAP:
            if '(sp)' in dest_reg_or_stack:
                temp_reg = self.allocator.get_temp_register()
                riscv_instructions.append(f"    {SHIFT_OPS_MAP[op]} {temp_reg}, {op1_reg}, {op2_reg}")
                self.allocator.store_to_stack_if_needed(temp_reg, dest_reg_or_stack, data_type, riscv_instructions)
            else:
                riscv_instructions.append(f"    {SHIFT_OPS_MAP[op]} {dest_reg_or_stack}, {op1_reg}, {op2_reg}")
        else:
            riscv_instructions.append(f"    # UNSUPPORTED SHIFT: {op}")
            
        return riscv_instructions
    
    def _get_or_load_operand(self, operand, data_type, riscv_instructions):
        """获取操作数寄存器，如果是立即数或在栈上，则加载到临时寄存器"""
        is_float = data_type in [DataType.F32, DataType.F64]
        
        # 检查是否为立即数
        if operand.isdigit() or (operand.startswith('-') and operand[1:].isdigit()):
            temp_reg = self.allocator.get_temp_register()
            riscv_instructions.append(f"    li {temp_reg}, {operand}")
            return temp_reg
        
        # 检查是否为浮点立即数
        if is_float:
            try:
                float_val = float(operand)
                int_bits = struct.unpack('>I', struct.pack('>f', float_val))[0]
                temp_reg_int = self.allocator.get_temp_register()
                temp_reg_float = self.allocator.get_temp_register()
                riscv_instructions.append(f"    li {temp_reg_int}, 0x{int_bits:08x}")
                riscv_instructions.append(f"    fmv.w.x {temp_reg_float}, {temp_reg_int}")
                return temp_reg_float
            except ValueError:
                pass # 不是浮点立即数

        # 检查是否为虚拟寄存器
        if operand.startswith('%'):
            reg_or_stack = self.allocator.get_physical_reg(operand)
            if reg_or_stack:
                if '(sp)' in reg_or_stack:
                    # 从栈加载
                    temp_reg = self.allocator.get_temp_register()
                    load_instr = self._get_load_instruction(data_type)
                    riscv_instructions.append(f"    {load_instr} {temp_reg}, {reg_or_stack}")
                    return temp_reg
                else:
                    # 已经是物理寄存器
                    return reg_or_stack
        
        # 如果都失败，返回None
        riscv_instructions.append(f"    # ERROR: Could not resolve operand {operand}")
        return None

    def _get_load_instruction(self, data_type):
        """根据数据类型返回对应的RISC-V加载指令"""
        if data_type == DataType.I1:
            return "lbu"  # 加载无符号字节
        elif data_type == DataType.I8:
            return "lb"   # 加载有符号字节
        elif data_type == DataType.I16:
            return "lh"   # 加载半字
        elif data_type == DataType.I32:
            return "lw"   # 加载字
        elif data_type == DataType.I64:
            return "ld"   # 加载双字
        elif data_type == DataType.F32:
            return "flw"  # 加载单精度浮点
        elif data_type == DataType.F64:
            return "fld"  # 加载双精度浮点
        else:
            return "lw"   # 默认使用字加载
    
    def _get_store_instruction(self, data_type):
        """根据数据类型返回对应的RISC-V存储指令"""
        if data_type in [DataType.I1, DataType.I8]:
            return "sb"   # 存储字节
        elif data_type == DataType.I16:
            return "sh"   # 存储半字
        elif data_type == DataType.I32:
            return "sw"   # 存储字
        elif data_type == DataType.I64:
            return "sd"   # 存储双字
        elif data_type == DataType.F32:
            return "fsw"  # 存储单精度浮点
        elif data_type == DataType.F64:
            return "fsd"  # 存储双精度浮点
        else:
            return "sw"   # 默认使用字存储

    def _translate_compare(self, instruction):
        """翻译比较指令"""
        riscv_instructions = []
        op = instruction.opcode
        result = instruction.result
        cond = instruction.operands[0]
        op1 = instruction.operands[1]
        op2 = instruction.operands[2]
        data_type = get_data_type(instruction.types[1])
        
        dest_reg_or_stack = self.allocator.allocate_register(result, DataType.I1, False)
        
        if op == 'icmp':
            op1_reg = self._get_or_load_operand(op1, data_type, riscv_instructions)
            op2_reg = self._get_or_load_operand(op2, data_type, riscv_instructions)
            
            # 目标寄存器
            dest_reg = dest_reg_or_stack
            if '(sp)' in dest_reg_or_stack:
                dest_reg = self.allocator.get_temp_register()

            if cond in CMP_OPS_MAP:
                riscv_op = CMP_OPS_MAP[cond]
                if riscv_op in ['slt', 'sltu']:
                    riscv_instructions.append(f"    {riscv_op} {dest_reg}, {op1_reg}, {op2_reg}")
                elif riscv_op == 'seqz': # eq -> sub, seqz
                    temp_sub = self.allocator.get_temp_register()
                    riscv_instructions.append(f"    sub {temp_sub}, {op1_reg}, {op2_reg}")
                    riscv_instructions.append(f"    seqz {dest_reg}, {temp_sub}")
                elif riscv_op == 'snez': # ne -> sub, snez
                    temp_sub = self.allocator.get_temp_register()
                    riscv_instructions.append(f"    sub {temp_sub}, {op1_reg}, {op2_reg}")
                    riscv_instructions.append(f"    snez {dest_reg}, {temp_sub}")
                elif riscv_op == 'sgt': # sgt -> slt op2, op1
                    riscv_instructions.append(f"    slt {dest_reg}, {op2_reg}, {op1_reg}")
                elif riscv_op == 'sge': # sge -> slt op1, op2; xori res, res, 1
                    riscv_instructions.append(f"    slt {dest_reg}, {op1_reg}, {op2_reg}")
                    riscv_instructions.append(f"    xori {dest_reg}, {dest_reg}, 1")
                elif riscv_op == 'sle': # sle -> slt op2, op1; xori res, res, 1
                    riscv_instructions.append(f"    slt {dest_reg}, {op2_reg}, {op1_reg}")
                    riscv_instructions.append(f"    xori {dest_reg}, {dest_reg}, 1")

            if '(sp)' in dest_reg_or_stack:
                riscv_instructions.append(f"    sw {dest_reg}, {dest_reg_or_stack}")

        elif op == 'fcmp':
            op1_reg = self._get_or_load_operand(op1, data_type, riscv_instructions)
            op2_reg = self._get_or_load_operand(op2, data_type, riscv_instructions)
            
            dest_reg = dest_reg_or_stack
            if '(sp)' in dest_reg_or_stack:
                dest_reg = self.allocator.get_temp_register()

            if cond in FLOAT_CMP_MAP:
                riscv_op = FLOAT_CMP_MAP[cond]
                riscv_instructions.append(f"    {riscv_op} {dest_reg}, {op1_reg}, {op2_reg}")
            
            if '(sp)' in dest_reg_or_stack:
                riscv_instructions.append(f"    sw {dest_reg}, {dest_reg_or_stack}")

        return riscv_instructions
    
    def _translate_branch(self, instruction):
        """翻译分支指令"""
        riscv_instructions = []
        opcode = instruction.opcode
        
        if opcode == 'br':
            if len(instruction.operands) == 1:  # 无条件跳转
                target_label = self.label_map.get(instruction.operands[0], "unknown_label")
                riscv_instructions.append(f"    j {target_label}")
            else:  # 条件跳转
                cond = instruction.operands[0]
                true_label = self.label_map.get(instruction.operands[1], "unknown_label")
                false_label = self.label_map.get(instruction.operands[2], "unknown_label")
                
                # 获取条件寄存器
                cond_reg = self._get_or_load_operand(cond, DataType.I1, riscv_instructions)
                
                # 生成条件跳转指令
                riscv_instructions.append(f"    bnez {cond_reg}, {true_label}")
                riscv_instructions.append(f"    j {false_label}")
        
        return riscv_instructions