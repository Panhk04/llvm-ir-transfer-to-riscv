"""
数据类型和常量定义模块
"""

from collections import namedtuple
from enum import Enum

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

# RISC-V寄存器定义
class RISCVRegisters:
    PARAM_REGS = ['a0', 'a1', 'a2', 'a3', 'a4', 'a5', 'a6', 'a7']
    TEMP_REGS = ['t0', 't1', 't2', 't3', 't4', 't5', 't6']
    SAVED_REGS = ['s0', 's1', 's2', 's3', 's4', 's5', 's6', 's7']
    FLOAT_REGS = ['ft0', 'ft1', 'ft2', 'ft3', 'ft4', 'ft5', 'ft6', 'ft7',
                  'fa0', 'fa1', 'fa2', 'fa3', 'fa4', 'fa5', 'fa6', 'fa7']

# 浮点运算映射
FLOAT_OPS_MAP = {
    'fadd': 'fadd.s',
    'fsub': 'fsub.s',
    'fmul': 'fmul.s',
    'fdiv': 'fdiv.s',
    'fcmp': 'feq.s'
}

# 整数运算映射
INT_OPS_MAP = {
    'add': 'add',
    'sub': 'sub', 
    'mul': 'mul',
    'sdiv': 'div',
    'and': 'and',
    'or': 'or',
    'xor': 'xor'
}

# 移位运算映射
SHIFT_OPS_MAP = {
    'shl': {'imm': 'slli', 'reg': 'sll'},
    'lshr': {'imm': 'srli', 'reg': 'srl'},
    'ashr': {'imm': 'srai', 'reg': 'sra'}
}

# 比较运算映射
CMP_OPS_MAP = {
    'eq': 'seqz',
    'ne': 'snez',
    'slt': 'slt',
    'sge': 'sgt',
    'sgt': 'sgt',
    'sle': 'slt',
    'ult': 'sltu',
    'uge': 'sgtu',
    'ugt': 'sgtu',
    'ule': 'sltu'
}

# 浮点比较运算映射
FLOAT_CMP_MAP = {
    'oeq': 'feq.s',
    'ogt': 'fgt.s',
    'oge': 'fge.s',
    'olt': 'flt.s',
    'ole': 'fle.s',
    'one': 'fne.s'
}

def get_data_type(type_str):
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

def calculate_type_size(type_str):
    """计算LLVM类型的字节大小"""
    # 处理数组类型 [4 x [2 x i32]]
    if type_str.startswith('[') and 'x' in type_str:
        # 解析多维数组
        import re
        
        # 提取所有数字
        numbers = re.findall(r'\[(\d+)\s+x', type_str)
        total_elements = 1
        for num in numbers:
            total_elements *= int(num)
        
        # 确定基础类型大小
        if 'i32' in type_str:
            element_size = 4
        elif 'i64' in type_str:
            element_size = 8
        elif 'i16' in type_str:
            element_size = 2
        elif 'i8' in type_str:
            element_size = 1
        elif 'float' in type_str:
            element_size = 4
        elif 'double' in type_str:
            element_size = 8
        else:
            element_size = 4  # 默认
        
        return total_elements * element_size
    
    # 处理基础类型
    elif type_str == 'i32':
        return 4
    elif type_str == 'i64':
        return 8
    elif type_str == 'i16':
        return 2
    elif type_str == 'i8':
        return 1
    elif type_str == 'float':
        return 4
    elif type_str == 'double':
        return 8
    else:
        return 4  # 默认