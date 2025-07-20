"""
重构后的LLVM IR到RISC-V汇编翻译器主模块
"""

import re
from ir_parsar import IRParser
from types_and_constants import Function, BasicBlock, Instruction, get_data_type, calculate_type_size
from register_allocator import RegisterAllocator
from optimizer import IROptimizer
from instruction_translator import InstructionTranslator

class OptimizedLLVMIRTranslator:
    def __init__(self):
        self.allocator = RegisterAllocator()
        self.optimizer = IROptimizer()
        self.instruction_translator = InstructionTranslator(self.allocator)
        self.label_map = {}
        self.current_function = None
    
    def translate(self, ir_code):
        """主翻译函数"""
        # 解析IR代码
        parser = IRParser()
        declarations, functions = parser.parse(ir_code)
        
        riscv_code = []
        
        # 处理全局变量声明
        global_vars = self._extract_global_variables(ir_code)
        if global_vars:
            riscv_code.extend(self._generate_data_section(global_vars))
        
        riscv_code.append(".text")
        
        # 检查是否有main函数并添加.globl指令
        for func in functions:
            if func.name == 'main':
                riscv_code.append(".globl main")
                break
        
        # 构建标签映射
        self._build_label_map(functions)
        self.instruction_translator.set_label_map(self.label_map)
        
        # 翻译每个函数
        for func in functions:
            self.current_function = func
            self.allocator.reset()
            self.allocator.analyze_liveness(func)
            
            # 应用优化
            self.optimizer.optimize_function(func)
            
            riscv_code.extend(self._translate_function(func))
            
        return "\n".join(riscv_code)
    
    def _translate_function(self, function):
        """翻译单个函数"""
        riscv_code = []
        func_label = function.name
        
        # 函数标签
        riscv_code.append(f"{func_label}:")
        
        # 预处理所有alloca指令以建立stack_frame映射
        self._preprocess_alloca_instructions(function)
        
        # 计算所需的栈空间，加上额外的临时变量空间
        base_stack_size = self.allocator.get_stack_size()
        # 为临时变量和溢出寄存器预留更多空间
        temp_space = max(200, len(function.blocks[0].instructions) * 8)  # 每条指令预留8字节
        total_stack_size = base_stack_size + temp_space
        
        # 确保栈对齐 (16字节对齐)，并预留函数调用空间
        aligned_size = (total_stack_size + 32 + 15) // 16 * 16  # 额外32字节用于ra/fp保存
        
        # 设置instruction_translator的栈大小
        self.instruction_translator.set_stack_size(aligned_size)
        
        # 函数序言 - 设置栈帧
        riscv_code.append("    # Function prologue")
        riscv_code.append(f"    addi sp, sp, -{aligned_size}")
        # 保存返回地址和帧指针到栈的最高地址
        riscv_code.append(f"    sw ra, {aligned_size-4}(sp)")
        riscv_code.append(f"    sw s0, {aligned_size-8}(sp)")
        riscv_code.append(f"    addi s0, sp, {aligned_size}")
        
        # 更新分配器的栈空间信息，避免冲突
        self.allocator.reserved_stack_top = aligned_size - 16  # 为ra/s0保留的空间
        
        # 添加零初始化代码（如果有未初始化的数组）
        zero_init_code = self._generate_array_zero_initialization()
        riscv_code.extend(zero_init_code)
        
        # 翻译每个基本块
        for block in function.blocks:
            # 块标签
            if block.name in self.label_map:
                riscv_code.append(f"{self.label_map[block.name]}:")
                
            # 翻译指令
            for inst in block.instructions:
                # 跳过alloca指令，因为已经在预处理中处理了
                if inst.opcode == 'alloca':
                    continue
                    
                riscv_inst = self.instruction_translator.translate_instruction(inst)
                riscv_code.extend(riscv_inst)
        
        # 检查是否已经有返回指令，避免重复添加
        has_ret_instruction = any(inst.opcode == 'ret' for block in function.blocks for inst in block.instructions)
        
        if not has_ret_instruction:
            # 如果没有返回指令，添加默认返回
            riscv_code.append("    # Default return")
            riscv_code.append("    li a0, 0")
            riscv_code.append("    # Function epilogue")
            riscv_code.append(f"    lw ra, {aligned_size-4}(sp)")
            riscv_code.append(f"    lw s0, {aligned_size-8}(sp)")
            riscv_code.append(f"    addi sp, sp, {aligned_size}")
            riscv_code.append("    ret")
        
        # 注意：如果有ret指令，函数尾声已经在instruction_translator中处理
        
        riscv_code.append("")  # 添加空行分隔函数
        return riscv_code
    
    def _preprocess_alloca_instructions(self, function):
        """预处理所有alloca指令，建立stack_frame映射"""
        alloca_vars = []
        
        for block in function.blocks:
            for i, inst in enumerate(block.instructions):
                if inst.opcode == 'alloca':
                    result_reg = inst.result
                    type_str = inst.types[0]
                    
                    # 解析数组类型和计算大小
                    size = calculate_type_size(type_str)
                    
                    # 更新栈偏移，为这个变量分配栈空间
                    self.allocator.stack_offset += size
                    # 确保栈对齐（4字节对齐）
                    if self.allocator.stack_offset % 4 != 0:
                        self.allocator.stack_offset = (self.allocator.stack_offset + 3) // 4 * 4
                    
                    stack_offset = self.allocator.stack_offset
                    
                    # 直接将栈偏移记录到stack_frame中
                    self.allocator.stack_frame[result_reg] = stack_offset
                    
                    # 记录alloca变量，用于后续检查是否需要零初始化
                    alloca_vars.append((result_reg, size, stack_offset))
        
        # 检查哪些alloca变量没有被显式初始化，需要零初始化
        self._check_and_init_uninitialized_arrays(function, alloca_vars)
    
    def _check_and_init_uninitialized_arrays(self, function, alloca_vars):
        """检查并初始化未初始化的数组"""
        # 先收集所有有初始化的变量
        initialized_vars = set()
        
        for block in function.blocks:
            for inst in block.instructions:
                if inst.opcode == 'getelementptr':
                    base_ptr = inst.operands[0]
                    gep_result = inst.result
                    
                    # 检查这个getelementptr的结果是否被store指令使用
                    for block2 in function.blocks:
                        for inst2 in block2.instructions:
                            if inst2.opcode == 'store' and len(inst2.operands) > 1 and inst2.operands[1] == gep_result:
                                initialized_vars.add(base_ptr)
                                break
        
        # 找出未初始化的数组变量
        self.uninitialized_arrays = []
        for var_name, size, stack_offset in alloca_vars:
            # 直接比较字符串，不需要类型转换
            if var_name not in initialized_vars:
                self.uninitialized_arrays.append((var_name, size, stack_offset))
                print(f"DEBUG: Found uninitialized array: {var_name}, size: {size}, offset: {stack_offset}")
            else:
                print(f"DEBUG: Array {var_name} has initialization")
    
    def _generate_array_zero_initialization(self):
        """生成未初始化数组的零初始化代码"""
        riscv_code = []
        
        if hasattr(self, 'uninitialized_arrays'):
            for var_name, size, stack_offset in self.uninitialized_arrays:
                riscv_code.append(f"    # Zero-initialize uninitialized array {var_name} ({size} bytes)")
                
                # 直接初始化数组（简化版本）
                for offset in range(0, size, 4):
                    riscv_code.append(f"    sw zero, {stack_offset - offset}(sp)")
        
        return riscv_code
    
    def _generate_data_section(self, global_vars):
        """生成数据段 - 支持数组类型"""
        data_section = [".data"]
        for var_name, var_type, var_value in global_vars:
            data_section.append(f".globl {var_name}")
            data_section.append(f"{var_name}:")
            
            if var_type.startswith('['):
                # 数组类型处理：[5 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4]
                self._generate_array_data(data_section, var_type, var_value)
            elif var_type == 'i32':
                data_section.append(f"    .word {var_value}")
            elif var_type == 'i64':
                data_section.append(f"    .dword {var_value}")
            elif var_type == 'float':
                data_section.append(f"    .float {var_value}")
            elif var_type == 'double':
                data_section.append(f"    .double {var_value}")
            else:
                data_section.append(f"    .word {var_value}")  # 默认
        data_section.append("")  # 添加空行分隔
        return data_section
    
    def _generate_array_data(self, data_section, array_type, array_value):
        """生成数组数据"""
        # 解析数组初始值：[i32 0, i32 1, i32 2, i32 3, i32 4]
        if array_value.startswith('[') and array_value.endswith(']'):
            # 移除外层括号
            inner_content = array_value[1:-1].strip()
            # 分割各个元素：i32 0, i32 1, i32 2, i32 3, i32 4
            elements = inner_content.split(',')
            
            for element in elements:
                element = element.strip()
                # 解析每个元素：i32 0
                parts = element.split()
                if len(parts) >= 2:
                    elem_type = parts[0]
                    elem_value = parts[1]
                    
                    if elem_type == 'i32':
                        data_section.append(f"    .word {elem_value}")
                    elif elem_type == 'i64':
                        data_section.append(f"    .dword {elem_value}")
                    elif elem_type == 'float':
                        data_section.append(f"    .float {elem_value}")
                    else:
                        data_section.append(f"    .word {elem_value}")  # 默认
        else:
            # 如果不是标准格式，使用默认处理
            data_section.append(f"    .word 0")  # 默认零初始化
    
    def _build_label_map(self, functions):
        """构建基本块标签映射"""
        self.label_map = {}
        for func in functions:
            for block in func.blocks:
                self.label_map[block.name] = f".{func.name}_{block.name[1:]}"
    
    def _extract_global_variables(self, ir_code):
        """从LLVM IR代码中提取全局变量声明 - 支持数组类型"""
        global_vars = []
        lines = ir_code.strip().split('\n')
        
        for line in lines:
            line = line.strip()
            # 改进的正则表达式，支持数组类型：@g_a = dso_local global [5 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4]
            global_match = re.match(r'@(\w+)\s*=\s*(?:dso_local\s+)?global\s+(.+)', line)
            if global_match:
                var_name = global_match.group(1)
                rest_content = global_match.group(2)
                
                # 解析类型和初始值
                if rest_content.startswith('['):
                    # 数组类型：[5 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4]
                    # 找到第一个 ] 后面的空格，分离类型和初始值
                    type_end = rest_content.find(']')
                    if type_end != -1:
                        var_type = rest_content[:type_end + 1]  # [5 x i32]
                        var_value = rest_content[type_end + 1:].strip()  # [i32 0, i32 1, i32 2, i32 3, i32 4]
                        global_vars.append((var_name, var_type, var_value))
                else:
                    # 简单类型：i32 3
                    parts = rest_content.split(' ', 1)
                    if len(parts) >= 2:
                        var_type = parts[0]
                        var_value = parts[1]
                        global_vars.append((var_name, var_type, var_value))
        
        return global_vars