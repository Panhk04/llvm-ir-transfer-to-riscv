import re
from collections import namedtuple

# 定义数据结构
Function = namedtuple('Function', ['name', 'return_type', 'params', 'blocks'])
BasicBlock = namedtuple('BasicBlock', ['name', 'instructions'])
Instruction = namedtuple('Instruction', ['opcode', 'operands', 'result', 'types'])

class IRParser:
    def __init__(self):
        self.declarations = []
        self.functions = []
        
    def parse(self, ir_code):
        """解析LLVM IR代码"""
        lines = ir_code.strip().split('\n')
        current_func = None
        current_block = None
        
        for line in lines:
            line = line.strip()
            
            # 跳过空行和注释
            if not line or line.startswith(';'):
                continue
                
            # 函数声明
            if line.startswith('declare'):
                self.declarations.append(line)
                continue
                
            # 函数定义
            if line.startswith('define'):
                match = re.match(r'define\s+(\w+)\s+@(\w+)\(\)\s*{', line)
                if match:
                    return_type = match.group(1)
                    func_name = match.group(2)
                    current_func = Function(
                        name=func_name, 
                        return_type=return_type, 
                        params=[], 
                        blocks=[]
                    )
                    continue
            
            # 函数结束
            if line == '}':
                if current_func:
                    self.functions.append(current_func)
                    current_func = None
                    current_block = None
                continue
                
            # 基本块
            if line.endswith(':'):
                block_name = line[:-1].strip()
                if current_func:
                    current_block = BasicBlock(name=block_name, instructions=[])
                    current_func.blocks.append(current_block)
                continue
                
            # 解析指令
            if current_block:
                # 处理ret指令
                ret_match = re.match(r'ret\s+(\w+)\s+(\w+|\d+)', line)
                if ret_match:
                    inst = Instruction(
                        opcode='ret',
                        operands=[ret_match.group(2)],
                        result=None,
                        types=[ret_match.group(1)]
                    )
                    current_block.instructions.append(inst)
                    continue
                
                # 处理二元运算指令 (例如: %sum = add i32 %a, %b)
                binop_match = re.match(r'(\%[\w\d]+)\s*=\s*(\w+)\s+(\w+)\s+(\%[\w\d]+|\d+),\s+(\%[\w\d]+|\d+)', line)
                if binop_match:
                    inst = Instruction(
                        opcode=binop_match.group(2),
                        operands=[binop_match.group(4), binop_match.group(5)],
                        result=binop_match.group(1),
                        types=[binop_match.group(3)]
                    )
                    current_block.instructions.append(inst)
                    continue
                
                # 处理load指令 (例如: %1 = load i32, i32* %a)
                load_match = re.match(r'(\%[\w\d]+)\s*=\s*load\s+(\w+),\s+(\w+\*)\s+(\%[\w\d]+)', line)
                if load_match:
                    inst = Instruction(
                        opcode='load',
                        operands=[load_match.group(4)],
                        result=load_match.group(1),
                        types=[load_match.group(2), load_match.group(3)]
                    )
                    current_block.instructions.append(inst)
                    continue
                
                # 处理store指令 (例如: store i32 3, i32* %a)
                store_match = re.match(r'store\s+(\w+)\s+(\%[\w\d]+|\d+),\s+(\w+\*)\s+(\%[\w\d]+)', line)
                if store_match:
                    inst = Instruction(
                        opcode='store',
                        operands=[store_match.group(2), store_match.group(4)],
                        result=None,
                        types=[store_match.group(1), store_match.group(3)]
                    )
                    current_block.instructions.append(inst)
                    continue
                
                # 处理alloca指令 (例如: %a = alloca i32)
                alloca_match = re.match(r'(\%[\w\d]+)\s*=\s*alloca\s+(\w+)', line)
                if alloca_match:
                    inst = Instruction(
                        opcode='alloca',
                        operands=[],
                        result=alloca_match.group(1),
                        types=[alloca_match.group(2)]
                    )
                    current_block.instructions.append(inst)
                    continue
                
                # 处理比较指令 (例如: %cmp = icmp eq i32 %a, %b)
                icmp_match = re.match(r'(\%[\w\d]+)\s*=\s*icmp\s+(\w+)\s+(\w+)\s+(\%[\w\d]+|\d+),\s+(\%[\w\d]+|\d+)', line)
                if icmp_match:
                    inst = Instruction(
                        opcode='icmp',
                        operands=[icmp_match.group(4), icmp_match.group(5)],
                        result=icmp_match.group(1),
                        types=[icmp_match.group(3), icmp_match.group(2)]
                    )
                    current_block.instructions.append(inst)
                    continue
                
                # 处理分支指令 (例如: br i1 %cmp, label %true_block, label %false_block)
                br_match = re.match(r'br\s+(\w+)\s+(\%[\w\d]+),\s+label\s+(\%[\w\d]+),\s+label\s+(\%[\w\d]+)', line)
                if br_match:
                    inst = Instruction(
                        opcode='br',
                        operands=[br_match.group(2), br_match.group(3), br_match.group(4)],
                        result=None,
                        types=[br_match.group(1)]
                    )
                    current_block.instructions.append(inst)
                    continue
                
                # 处理无条件跳转 (例如: br label %next_block)
                jmp_match = re.match(r'br\s+label\s+(\%[\w\d]+)', line)
                if jmp_match:
                    inst = Instruction(
                        opcode='jmp',
                        operands=[jmp_match.group(1)],
                        result=None,
                        types=[]
                    )
                    current_block.instructions.append(inst)
                    continue
                
                # 处理函数调用 (例如: %result = call i32 @add(i32 %a, i32 %b))
                call_match = re.match(r'(\%[\w\d]+)?\s*=\s*call\s+(\w+)\s+@(\w+)\((.+)\)', line)
                if call_match:
                    result_var = call_match.group(1)
                    return_type = call_match.group(2)
                    func_name = call_match.group(3)
                    args_str = call_match.group(4)
                    
                    # 解析参数
                    args = []
                    arg_types = []
                    if args_str:
                        arg_parts = [arg.strip() for arg in args_str.split(',')]
                        for arg in arg_parts:
                            type_val = arg.split()
                            if len(type_val) == 2:
                                arg_types.append(type_val[0])
                                args.append(type_val[1])
                    
                    inst = Instruction(
                        opcode='call',
                        operands=[func_name] + args,
                        result=result_var,
                        types=[return_type] + arg_types
                    )
                    current_block.instructions.append(inst)
                    continue
        
        return self.declarations, self.functions

    def get_function(self, name):
        """根据名称获取函数"""
        for func in self.functions:
            if func.name == name:
                return func
        return None

# 测试代码
if __name__ == "__main__":
    # 测试用例
    test_ir = """
declare i32 @get_char()
declare float @get_float()
declare i32 @get_int()
declare void @print_char(i32)
declare void @print_float(float)
declare void @print_int(i32)

define dso_local i32 @main() {
entry:
    %a = alloca i32
    store i32 5, i32* %a
    %b = load i32, i32* %a
    %sum = add i32 %b, 3
    ret i32 %sum
}

define dso_local i32 @add(i32 %x, i32 %y) {
entry:
    %result = add i32 %x, %y
    ret i32 %result
}
"""
    
    parser = IRParser()
    declarations, functions = parser.parse(test_ir)
    
    print("Declarations:")
    for decl in declarations:
        print(f"  {decl}")
    
    print("\nFunctions:")
    for func in functions:
        print(f"Function: {func.name}, Return type: {func.return_type}")
        for block in func.blocks:
            print(f"  Block: {block.name}")
            for inst in block.instructions:
                print(f"    {inst.opcode}: result={inst.result}, operands={inst.operands}, types={inst.types}")