class RISCVEmitter:
    def __init__(self, register_allocator):
        self.register_allocator = register_allocator
        self.code = []
        self.current_block = None
        self.blocks = {}
        self.global_vars = {}

    def emit_program_start(self):
        """发射程序起始部分"""
        self.code.append(".text")
        self.code.append(".globl main")

    def emit_function_start(self, func_name, args):
        """发射函数起始部分"""
        self.code.append(f"\n{func_name}:")
        # 保存调用者保存的寄存器
        self.code.append("addi sp, sp, -16")
        self.code.append("sw ra, 12(sp)")
        self.code.append("sw s0, 8(sp)")
        self.code.append("addi s0, sp, 16")

    def emit_function_end(self):
        """发射函数结束部分"""
        # 恢复寄存器并返回
        self.code.append("lw ra, 12(sp)")
        self.code.append("lw s0, 8(sp)")
        self.code.append("addi sp, sp, 16")
        self.code.append("ret")

    def emit_basic_block(self, block_name):
        """发射基本块标签"""
        self.current_block = block_name
        self.code.append(f"{block_name}:")

    def emit_return(self, value):
        """发射返回指令"""
        if value == "void":
            self.code.append("li a0, 0")  # 返回0
        else:
            # 将返回值移到a0寄存器
            reg = self.register_allocator.get_register(value)
            self.code.append(f"mv a0, {reg}")
        self.code.append("j end_function")  # 跳转到函数结束

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

    def emit_global_variable(self, name, type_, initializer):
        """发射全局变量定义"""
        self.global_vars[name] = {
            'type': type_,
            'initializer': initializer
        }

    def finalize(self):
        """完成代码生成，添加全局变量和结束标记"""
        # 添加全局变量部分
        if self.global_vars:
            self.code.append("\n.data")
            for name, info in self.global_vars.items():
                if info['type'] == 'i32':
                    self.code.append(f"{name}: .word {info['initializer']}")
                elif info['type'] == 'float':
                    self.code.append(f"{name}: .float {info['initializer']}")
                elif info['type'] == 'array':
                    # 处理数组类型
                    elements = ', '.join(map(str, info['initializer']))
                    self.code.append(f"{name}: .word {elements}")

        # 添加结束标记
        self.code.append("\nend_function:")
        
        return "\n".join(self.code)