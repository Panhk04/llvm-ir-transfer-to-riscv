import re
from collections import namedtuple

# 定义数据结构 - 替换为可变的类
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

class IRParser:
    def __init__(self):
        self.declarations = []
        self.functions = []
        
    def parse(self, ir_code):
        """解析LLVM IR代码"""
        lines = ir_code.strip().split('\n')
        current_func = None
        current_block = None
        current_blocks = []
        
        for line in lines:
            line = line.strip()
            
            # 跳过空行和注释
            if not line or line.startswith(';'):
                continue
                
            # 函数声明
            if line.startswith('declare'):
                self.declarations.append(line)
                continue
                
            # 函数定义 - 修复正则表达式以支持 dso_local 等修饰符
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
            
            # 函数结束
            if line == '}':
                if current_func:
                    # 确保当前块被添加
                    if current_block and current_block not in current_blocks:
                        current_blocks.append(current_block)
                    
                    # 创建函数对象
                    func_obj = Function(
                        name=current_func['name'],
                        return_type=current_func['return_type'],
                        params=current_func['params'],
                        blocks=current_blocks
                    )
                    self.functions.append(func_obj)
                    current_func = None
                    current_block = None
                    current_blocks = []
                continue
                
            # 基本块 - 改进基本块识别
            if line.endswith(':'):
                # 保存之前的块
                if current_block:
                    current_blocks.append(current_block)
                
                block_name = line[:-1].strip()
                current_block = BasicBlock(name=block_name, instructions=[])
                continue
                
            # 解析指令
            if current_func and current_block:
                # 处理getelementptr指令 - 完全重写以正确解析两种模式
                if 'getelementptr' in line and '=' in line:
                    result_var = line.split('=')[0].strip()
                    
                    # 解析getelementptr的完整语法
                    # 模式1: getelementptr [4 x [2 x i32]], [4 x [2 x i32]]* %ptr, i32 0, i32 row, i32 col
                    # 模式2: getelementptr i32, i32* %ptr, i32 offset
                    
                    # 查找基础指针（%开头的变量）
                    ptr_vars = re.findall(r'(\%[\w\d]+)', line)
                    if len(ptr_vars) < 2:  # 至少需要结果变量和基础指针
                        continue
                        
                    base_ptr = ptr_vars[1]  # 第二个是基础指针
                    
                    # 判断是哪种模式
                    if '[' in line and 'x' in line and line.count('i32') > 2:
                        # 模式1: 数组索引模式 - 有数组类型声明且多个i32索引
                        # 提取所有i32索引
                        indices = re.findall(r'i32\s+(-?\d+)', line)
                        
                        # 保存类型信息
                        array_type_match = re.search(r'\[([^\]]+)\]', line)
                        if array_type_match:
                            base_type = f"[{array_type_match.group(1)}]"
                        else:
                            base_type = 'array'
                        
                        inst = Instruction(
                            opcode='getelementptr',
                            operands=[base_ptr] + indices,
                            result=result_var,
                            types=[base_type, 'array_index']  # 标记为数组索引模式
                        )
                    else:
                        # 模式2: 指针偏移模式 - 简单的i32指针加偏移
                        # 只提取最后一个i32数字作为偏移
                        offset_match = re.search(r'i32\s+(-?\d+)(?!.*i32)', line)  # 找最后一个i32数字
                        if offset_match:
                            offset = offset_match.group(1)
                        else:
                            offset = '0'
                        
                        inst = Instruction(
                            opcode='getelementptr',
                            operands=[base_ptr, offset],
                            result=result_var,
                            types=['i32', 'pointer_offset']  # 标记为指针偏移模式
                        )
                    
                    current_block.instructions.append(inst)
                    continue
                
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
                
                # 处理二元运算指令 - 改进数字匹配
                binop_match = re.match(r'(\%[\w\d]+)\s*=\s*(\w+)\s+(\w+)\s+(-?\%?[\w\d]+|-?\d+),\s+(-?\%?[\w\d]+|-?\d+)', line)
                if binop_match:
                    inst = Instruction(
                        opcode=binop_match.group(2),
                        operands=[binop_match.group(4), binop_match.group(5)],
                        result=binop_match.group(1),
                        types=[binop_match.group(3)]
                    )
                    current_block.instructions.append(inst)
                    continue
                
                # 处理load指令 - 支持全局变量和局部变量
                # 局部变量: %1 = load i32, i32* %a
                # 全局变量: %1 = load i32, i32* @global_var
                load_match = re.match(r'(\%[\w\d]+)\s*=\s*load\s+(\w+),\s+(\w+\*)\s+(\%[\w\d]+|@[\w\d_]+)', line)
                if load_match:
                    inst = Instruction(
                        opcode='load',
                        operands=[load_match.group(4)],
                        result=load_match.group(1),
                        types=[load_match.group(2), load_match.group(3)]
                    )
                    current_block.instructions.append(inst)
                    continue
                
                # 处理store指令 - 改进数字匹配
                store_match = re.match(r'store\s+(\w+)\s+(-?\%?[\w\d]+|-?\d+),\s+(\w+\*)\s+(\%[\w\d]+)', line)
                if store_match:
                    inst = Instruction(
                        opcode='store',
                        operands=[store_match.group(2), store_match.group(4)],
                        result=None,
                        types=[store_match.group(1), store_match.group(3)]
                    )
                    current_block.instructions.append(inst)
                    continue
                
                # 处理alloca指令 - 修复以支持复杂类型如数组
                alloca_match = re.match(r'(\%[\w\d]+)\s*=\s*alloca\s+(.+)', line)
                if alloca_match:
                    result_var = alloca_match.group(1)
                    type_str = alloca_match.group(2).strip()
                    
                    inst = Instruction(
                        opcode='alloca',
                        operands=[],
                        result=result_var,
                        types=[type_str]
                    )
                    current_block.instructions.append(inst)
                    continue
                
                # 处理比较指令 - 改进数字匹配
                icmp_match = re.match(r'(\%[\w\d]+)\s*=\s*icmp\s+(\w+)\s+(\w+)\s+(-?\%?[\w\d]+|-?\d+),\s+(-?\%?[\w\d]+|-?\d+)', line)
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
        
        # 确保最后一个函数被处理
        if current_func:
            if current_block and current_block not in current_blocks:
                current_blocks.append(current_block)
            
            func_obj = Function(
                name=current_func['name'],
                return_type=current_func['return_type'],
                params=current_func['params'],
                blocks=current_blocks
            )
            self.functions.append(func_obj)
        
        return self.declarations, self.functions

    def get_function(self, name):
        """根据名称获取函数"""
        for func in self.functions:
            if func.name == name:
                return func
        return None

# 测试代码
if __name__ == "__main__":
    # 测试用例 - 测试实际的IR
    test_ir = """
define dso_local i32 @main() {
entry:
    ret i32 3
}
"""
    
    parser = IRParser()
    declarations, functions = parser.parse(test_ir)
    
    print("=== 解析结果调试 ===")
    print(f"声明数量: {len(declarations)}")
    print(f"函数数量: {len(functions)}")
    
    print("\nFunctions:")
    for func in functions:
        print(f"Function: {func.name}, Return type: {func.return_type}")
        print(f"  块数量: {len(func.blocks)}")
        for block in func.blocks:
            print(f"  Block: {block.name}")
            print(f"    指令数量: {len(block.instructions)}")
            for inst in block.instructions:
                print(f"    {inst.opcode}: result={inst.result}, operands={inst.operands}, types={inst.types}")
                
    # 验证解析结果
    if len(functions) > 0:
        main_func = functions[0]
        if len(main_func.blocks) > 0:
            entry_block = main_func.blocks[0]
            if len(entry_block.instructions) > 0:
                ret_inst = entry_block.instructions[0]
                print(f"\n=== 关键验证 ===")
                print(f"ret指令类型: {ret_inst.types[0]}")
                print(f"ret指令操作数: {ret_inst.operands[0]}")
                print(f"操作数是否为数字: {ret_inst.operands[0].isdigit()}")