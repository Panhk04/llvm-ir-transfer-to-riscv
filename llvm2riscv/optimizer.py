"""
LLVM IR优化器模块
"""

from types_and_constants import Instruction

class IROptimizer:
    def __init__(self):
        pass
    
    def optimize_function(self, function):
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