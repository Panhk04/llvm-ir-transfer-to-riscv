"""
LLVM IR到RISC-V汇编翻译器主模块
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
    
    def _generate_data_section(self, global_vars):
        """生成数据段代码"""
        data_section = [".data"]
        for var_name, var_type, var_value in global_vars:
            data_section.append(f".globl {var_name}")
            data_section.append(f"{var_name}:")
            if var_type == 'i32':
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
        
        # 预处理所有alloca指令以建立stack_frame映射
        self._preprocess_alloca_instructions(function)
        
        # 计算所需的栈空间
        stack_size = self.allocator.get_stack_size()
        # 为数组和局部变量预留足够空间，确保至少128字节的栈空间
        min_stack_size = 128
        if stack_size < min_stack_size:
            stack_size = min_stack_size
        
        # 函数序言 - 设置栈帧
        if stack_size > 0:
            # 确保栈对齐 (16字节对齐)
            aligned_size = (stack_size + 15) // 16 * 16
            riscv_code.append("    # Function prologue")
            riscv_code.append(f"    addi sp, sp, -{aligned_size}")
            # 保存返回地址和帧指针
            riscv_code.append(f"    sw ra, {aligned_size-4}(sp)")
            riscv_code.append(f"    sw s0, {aligned_size-8}(sp)")
            riscv_code.append(f"    addi s0, sp, {aligned_size}")
        
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
        
        # 如果没有返回指令，添加默认返回
        if not any(inst.opcode == 'ret' for block in function.blocks for inst in block.instructions):
            riscv_code.append("    # Default return")
            riscv_code.append("    li a0, 0")
        
        # 函数尾声
        if stack_size > 0:
            aligned_size = (stack_size + 15) // 16 * 16
            riscv_code.append("    # Function epilogue")
            riscv_code.append(f"    lw ra, {aligned_size-4}(sp)")
            riscv_code.append(f"    lw s0, {aligned_size-8}(sp)")
            riscv_code.append(f"    addi sp, sp, {aligned_size}")
        
        riscv_code.append("    ret")
        riscv_code.append("")  # 添加空行分隔函数
        return riscv_code
    
    def _preprocess_alloca_instructions(self, function):
        """预处理所有alloca指令，建立stack_frame映射"""
        for block in function.blocks:
            for inst in block.instructions:
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
                temp_reg = self._get_temp_register()
                riscv_instructions.append(f"    # Calculate address for {base_ptr} at stack offset {stack_offset}")
                riscv_instructions.append(f"    addi {temp_reg}, sp, {stack_offset}")
                base_reg = temp_reg
            else:
                # 尝试从寄存器映射获取
                base_reg_or_stack = self.allocator.get_physical_reg(base_ptr)
                if base_reg_or_stack:
                    if '(sp)' in base_reg_or_stack:
                        # 基础指针在栈上，先加载到临时寄存器
                        temp_reg = self._get_temp_register()
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
                    temp_reg = self._get_temp_register()
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
                temp_reg = self._get_temp_register()
                riscv_instructions.append(f"    mv {temp_reg}, {base_reg}")
                self._store_to_stack_if_needed(temp_reg, dest_reg_or_stack, DataType.I32, riscv_instructions)
            else:
                riscv_instructions.append(f"    mv {dest_reg_or_stack}, {base_reg}")
        else:
            # 添加偏移
            if '(sp)' in dest_reg_or_stack:
                temp_reg = self._get_temp_register()
                riscv_instructions.append(f"    addi {temp_reg}, {base_reg}, {total_offset}")
                self._store_to_stack_if_needed(temp_reg, dest_reg_or_stack, DataType.I32, riscv_instructions)
            else:
                riscv_instructions.append(f"    addi {dest_reg_or_stack}, {base_reg}, {total_offset}")
        
        return riscv_instructions