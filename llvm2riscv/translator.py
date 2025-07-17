import re
from collections import namedtuple, defaultdict
from ir_parsar import IRParser

# 定义数据结构
Function = namedtuple('Function', ['name', 'return_type', 'params', 'blocks'])
BasicBlock = namedtuple('BasicBlock', ['name', 'instructions'])
Instruction = namedtuple('Instruction', ['opcode', 'operands', 'result', 'types'])

class LLVMIRTranslator:
    def __init__(self):
        self.reg_counter = 0
        self.function_reg_map = {}
        self.label_map = {}
        self.current_function = None
        self.stack_offset = 0
        self.stack_frame = {}
        
        # RISC-V寄存器分配
        self.param_regs = ['a0', 'a1', 'a2', 'a3', 'a4', 'a5', 'a6', 'a7']
        self.temp_regs = ['t0', 't1', 't2', 't3', 't4', 't5', 't6']
        self.saved_regs = ['s0', 's1', 's2', 's3', 's4', 's5', 's6', 's7']
        self.reg_map = {}
        
    def translate(self, ir_code):
        """主翻译函数"""
        # 解析IR代码
        parser = IRParser()
        declarations, functions = parser.parse(ir_code)
        
        riscv_code = []
        riscv_code.append(".text")
        
        # 处理函数声明
        for decl in declarations:
            if '@main' in decl:
                riscv_code.append(".globl main")
        
        # 构建标签映射
        self._build_label_map(functions)
        
        # 翻译每个函数
        for func in functions:
            self.current_function = func
            self.reg_map = {}
            self.stack_offset = 0
            self.stack_frame = {}
            riscv_code.extend(self._translate_function(func))
            
        return "\n".join(riscv_code)
    
    def _build_label_map(self, functions):
        """构建基本块标签映射"""
        self.label_map = {}
        for func in functions:
            for block in func.blocks:
                self.label_map[block.name] = f".{func.name}_{block.name[1:]}"
    
    def _get_physical_reg(self, virtual_reg):
        """为虚拟寄存器分配物理寄存器"""
        if virtual_reg not in self.reg_map:
            # 优先使用临时寄存器
            if self.temp_regs:
                reg = self.temp_regs.pop(0)
                self.reg_map[virtual_reg] = reg
            else:
                # 寄存器不足，使用栈空间
                if virtual_reg not in self.stack_frame:
                    self.stack_offset += 4
                    self.stack_frame[virtual_reg] = self.stack_offset
                return f"{self.stack_frame[virtual_reg]}(sp)"
        return self.reg_map[virtual_reg]
    
    def _translate_function(self, function):
        """翻译单个函数"""
        riscv_code = []
        func_label = function.name
        
        # 函数标签
        riscv_code.append(f"{func_label}:")
        
        # 函数序言
        riscv_code.append("    # Function prologue")
        if self.stack_offset > 0:
            riscv_code.append(f"    addi sp, sp, -{self.stack_offset}")
        
        # 翻译每个基本块
        for block in function.blocks:
            # 块标签
            if block.name in self.label_map:
                riscv_code.append(f"{self.label_map[block.name]}:")
                
            # 翻译指令
            for inst in block.instructions:
                riscv_code.extend(self._translate_instruction(inst))
        
        # 函数尾声
        if self.stack_offset > 0:
            riscv_code.append("    # Function epilogue")
            riscv_code.append(f"    addi sp, sp, {self.stack_offset}")
        
        riscv_code.append("")  # 添加空行分隔函数
        return riscv_code
    
    def _translate_instruction(self, instruction):
        """翻译单条指令"""
        riscv_instructions = []
        opcode = instruction.opcode
        
        # 返回指令
        if opcode == 'ret':
            return self._translate_ret(instruction)
        
        # 加载指令
        elif opcode == 'load':
            return self._translate_load(instruction)
        
        # 存储指令
        elif opcode == 'store':
            return self._translate_store(instruction)
        
        # 内存分配
        elif opcode == 'alloca':
            return self._translate_alloca(instruction)
        
        # 算术指令
        elif opcode in ['add', 'sub', 'mul', 'sdiv', 'and', 'or', 'xor']:
            return self._translate_arithmetic(instruction)
        
        # 移位指令
        elif opcode in ['shl', 'lshr', 'ashr']:
            return self._translate_shift(instruction)
        
        # 比较指令
        elif opcode == 'icmp':
            return self._translate_icmp(instruction)
        
        # 分支指令
        elif opcode in ['br', 'jmp']:
            return self._translate_branch(instruction)
        
        # 函数调用
        elif opcode == 'call':
            return self._translate_call(instruction)
        
        # 未支持指令
        riscv_instructions.append(f"    # UNSUPPORTED: {instruction}")
        return riscv_instructions
    
    def _translate_ret(self, instruction):
        """翻译返回指令"""
        riscv_instructions = []
        ret_value = instruction.operands[0]
        
        # 处理返回值
        if ret_value.isdigit() or (ret_value.startswith('-') and ret_value[1:].isdigit()):
            # 加载立即数到返回寄存器
            riscv_instructions.append(f"    li a0, {ret_value}")
        elif ret_value.startswith('%'):
            # 从虚拟寄存器加载
            reg = self._get_physical_reg(ret_value)
            riscv_instructions.append(f"    mv a0, {reg}")
        
        # 函数返回
        riscv_instructions.append("    ret")
        return riscv_instructions
    
    def _translate_load(self, instruction):
        """翻译加载指令"""
        riscv_instructions = []
        result_reg = instruction.result
        src_ptr = instruction.operands[0]
        data_type = instruction.types[0]
        
        # 获取目标寄存器
        dest_reg = self._get_physical_reg(result_reg)
        
        # 获取源指针
        src_reg = self._get_physical_reg(src_ptr)
        
        # 根据数据类型选择加载指令
        if data_type == 'i32':
            riscv_instructions.append(f"    lw {dest_reg}, 0({src_reg})")
        elif data_type == 'i16':
            riscv_instructions.append(f"    lh {dest_reg}, 0({src_reg})")
        elif data_type == 'i8':
            riscv_instructions.append(f"    lb {dest_reg}, 0({src_reg})")
        else:
            riscv_instructions.append(f"    # UNSUPPORTED LOAD TYPE: {data_type}")
        
        return riscv_instructions
    
    def _translate_store(self, instruction):
        """翻译存储指令"""
        riscv_instructions = []
        value = instruction.operands[0]
        dest_ptr = instruction.operands[1]
        data_type = instruction.types[0]
        
        # 获取目标指针
        ptr_reg = self._get_physical_reg(dest_ptr)
        
        # 处理存储的值
        if value.isdigit() or (value.startswith('-') and value[1:].isdigit()):
            # 立即数，加载到临时寄存器
            temp_reg = self._get_physical_reg(f"%temp_{len(self.reg_map)}")
            riscv_instructions.append(f"    li {temp_reg}, {value}")
            value_reg = temp_reg
        elif value.startswith('%'):
            # 从虚拟寄存器获取
            value_reg = self._get_physical_reg(value)
        else:
            value_reg = value
        
        # 根据数据类型选择存储指令
        if data_type == 'i32':
            riscv_instructions.append(f"    sw {value_reg}, 0({ptr_reg})")
        elif data_type == 'i16':
            riscv_instructions.append(f"    sh {value_reg}, 0({ptr_reg})")
        elif data_type == 'i8':
            riscv_instructions.append(f"    sb {value_reg}, 0({ptr_reg})")
        else:
            riscv_instructions.append(f"    # UNSUPPORTED STORE TYPE: {data_type}")
        
        return riscv_instructions
    
    def _translate_alloca(self, instruction):
        """翻译内存分配指令"""
        riscv_instructions = []
        result_reg = instruction.result
        data_type = instruction.types[0]
        
        # 计算分配大小
        if data_type == 'i32':
            size = 4
        elif data_type == 'i16':
            size = 2
        elif data_type == 'i8':
            size = 1
        else:
            size = 4  # 默认为4字节
        
        # 更新栈偏移
        self.stack_offset += size
        stack_offset = self.stack_offset
        
        # 将栈地址保存到寄存器
        dest_reg = self._get_physical_reg(result_reg)
        riscv_instructions.append(f"    # Allocate {size} bytes on stack")
        riscv_instructions.append(f"    addi {dest_reg}, sp, {stack_offset}")
        
        return riscv_instructions
    
    def _translate_arithmetic(self, instruction):
        """翻译算术指令"""
        riscv_instructions = []
        op = instruction.opcode
        result = instruction.result
        op1 = instruction.operands[0]
        op2 = instruction.operands[1]
        data_type = instruction.types[0]
        
        # 获取目标寄存器
        dest_reg = self._get_physical_reg(result)
        
        # 处理操作数1
        if op1.isdigit() or (op1.startswith('-') and op1[1:].isdigit()):
            # 立即数，加载到临时寄存器
            temp_reg1 = self._get_physical_reg(f"%temp1_{len(self.reg_map)}")
            riscv_instructions.append(f"    li {temp_reg1}, {op1}")
            op1_reg = temp_reg1
        elif op1.startswith('%'):
            op1_reg = self._get_physical_reg(op1)
        else:
            op1_reg = op1
        
        # 处理操作数2
        if op2.isdigit() or (op2.startswith('-') and op2[1:].isdigit()):
            # 如果是立即数，特殊处理
            imm = int(op2)
            # 对于ADDI指令，立即数范围是-2048到2047
            if op in ['add', 'sub'] and -2048 <= imm <= 2047:
                if op == 'add':
                    riscv_instructions.append(f"    addi {dest_reg}, {op1_reg}, {imm}")
                else:  # sub
                    riscv_instructions.append(f"    addi {dest_reg}, {op1_reg}, {-imm}")
                return riscv_instructions
            else:
                # 加载到临时寄存器
                temp_reg2 = self._get_physical_reg(f"%temp2_{len(self.reg_map)}")
                riscv_instructions.append(f"    li {temp_reg2}, {op2}")
                op2_reg = temp_reg2
        elif op2.startswith('%'):
            op2_reg = self._get_physical_reg(op2)
        else:
            op2_reg = op2
        
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
            riscv_instructions.append(f"    {op_map[op]} {dest_reg}, {op1_reg}, {op2_reg}")
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
        
        # 获取目标寄存器
        dest_reg = self._get_physical_reg(result)
        
        # 处理操作数1
        if op1.startswith('%'):
            op1_reg = self._get_physical_reg(op1)
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
                temp_reg2 = self._get_physical_reg(f"%temp2_{len(self.reg_map)}")
                riscv_instructions.append(f"    li {temp_reg2}, {op2}")
                op2_reg = temp_reg2
        elif op2.startswith('%'):
            op2_reg = self._get_physical_reg(op2)
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
    
    def _translate_icmp(self, instruction):
        """翻译比较指令"""
        riscv_instructions = []
        cmp_type = instruction.types[1]  # 比较类型
        result = instruction.result
        op1 = instruction.operands[0]
        op2 = instruction.operands[1]
        
        # 获取目标寄存器
        dest_reg = self._get_physical_reg(result)
        
        # 处理操作数1
        if op1.startswith('%'):
            op1_reg = self._get_physical_reg(op1)
        else:
            op1_reg = op1
        
        # 处理操作数2
        if op2.isdigit() or (op2.startswith('-') and op2[1:].isdigit()):
            # 加载到临时寄存器
            temp_reg2 = self._get_physical_reg(f"%temp2_{len(self.reg_map)}")
            riscv_instructions.append(f"    li {temp_reg2}, {op2}")
            op2_reg = temp_reg2
        elif op2.startswith('%'):
            op2_reg = self._get_physical_reg(op2)
        else:
            op2_reg = op2
        
        # 映射比较类型
        cmp_map = {
            'eq': ('beq', 'bne'),
            'ne': ('bne', 'beq'),
            'slt': ('slt', 'sge'),
            'sge': ('sge', 'slt'),
            'sgt': ('sgt', 'sle'),
            'sle': ('sle', 'sgt'),
            'ult': ('sltu', 'sgeu'),
            'uge': ('sgeu', 'sltu'),
            'ugt': ('sgtu', 'sleu'),
            'ule': ('sleu', 'sgtu')
        }
        
        if cmp_type in cmp_map:
            # 使用比较和分支指令实现
            true_label = f".cmp_true_{len(self.label_map)}"
            false_label = f".cmp_false_{len(self.label_map)}"
            end_label = f".cmp_end_{len(self.label_map)}"
            
            # 比较操作
            cmp_instr, cmp_inv = cmp_map[cmp_type]
            riscv_instructions.append(f"    {cmp_instr} {op1_reg}, {op2_reg}, {true_label}")
            riscv_instructions.append(f"    j {false_label}")
            
            # 真分支
            riscv_instructions.append(f"{true_label}:")
            riscv_instructions.append(f"    li {dest_reg}, 1")
            riscv_instructions.append(f"    j {end_label}")
            
            # 假分支
            riscv_instructions.append(f"{false_label}:")
            riscv_instructions.append(f"    li {dest_reg}, 0")
            
            # 结束标签
            riscv_instructions.append(f"{end_label}:")
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
            cond_reg = self._get_physical_reg(cond)
            
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
        for i, reg in enumerate(self.temp_regs):
            riscv_instructions.append(f"    sw {reg}, {4*i}(sp)")
        
        # 设置参数
        for i, arg in enumerate(args):
            if i < len(self.param_regs):
                if arg.startswith('%'):
                    arg_reg = self._get_physical_reg(arg)
                    riscv_instructions.append(f"    mv {self.param_regs[i]}, {arg_reg}")
                else:
                    riscv_instructions.append(f"    li {self.param_regs[i]}, {arg}")
        
        # 函数调用
        riscv_instructions.append(f"    call {func_name}")
        
        # 恢复调用者保存的寄存器
        riscv_instructions.append("    # Restore caller-saved registers")
        for i, reg in enumerate(self.temp_regs):
            riscv_instructions.append(f"    lw {reg}, {4*i}(sp)")
        
        # 处理返回值
        if result:
            dest_reg = self._get_physical_reg(result)
            riscv_instructions.append(f"    mv {dest_reg}, a0")
        
        return riscv_instructions

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
    ret i32 1
    
false_block:
    ret i32 0
}
"""

    translator = LLVMIRTranslator()
    riscv_code = translator.translate(sample_ir)
    
    print("Generated RISC-V Assembly:")
    print(riscv_code)