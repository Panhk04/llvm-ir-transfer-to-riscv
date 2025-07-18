"""
RISC-V寄存器分配器模块
"""

from collections import defaultdict
from types_and_constants import DataType, RISCVRegisters

class RegisterAllocator:
    def __init__(self):
        # RISC-V 寄存器资源
        self.param_regs = RISCVRegisters.PARAM_REGS.copy()
        self.temp_regs = RISCVRegisters.TEMP_REGS.copy()
        self.saved_regs = RISCVRegisters.SAVED_REGS.copy()
        self.float_regs = RISCVRegisters.FLOAT_REGS.copy()
        
        # 寄存器映射和状态
        self.reg_map = {}
        self.reg_in_use = defaultdict(bool)
        self.virtual_regs = []
        self.liveness = {}
        
        # 栈帧信息
        self.stack_offset = 0
        self.stack_frame = {}
        self.param_offset = 0
        self.saved_regs_offset = {}
        self.temp_stack_offset = 0  # 临时栈偏移
    
    def reset(self):
        """重置寄存器分配器状态"""
        self.reg_map = {}
        self.reg_in_use = defaultdict(bool)
        self.virtual_regs = []
        self.liveness = {}
        self.stack_offset = 0
        self.stack_frame = {}
        self.param_offset = 0
        self.saved_regs_offset = {}
        self.temp_stack_offset = 0
    
    def analyze_liveness(self, function):
        """分析虚拟寄存器的活跃范围"""
        # 简化的活跃变量分析
        self.virtual_regs = []
        
        for block in function.blocks:
            for inst in block.instructions:
                # 记录定义
                if inst.result and inst.result.startswith('%'):
                    if inst.result not in self.virtual_regs:
                        self.virtual_regs.append(inst.result)
                        self.liveness[inst.result] = {"def": [], "use": []}
                    self.liveness[inst.result]["def"].append(inst)
                
                # 记录使用
                for op in inst.operands:
                    if op.startswith('%') and op in self.liveness:
                        self.liveness[op]["use"].append(inst)
    
    def allocate_register(self, virtual_reg, data_type, is_float=False):
        """为虚拟寄存器分配物理寄存器"""
        if virtual_reg in self.reg_map:
            return self.reg_map[virtual_reg]
        
        # 强制检查数据类型，确保整数类型不会分配浮点寄存器
        if data_type in [DataType.I1, DataType.I8, DataType.I16, DataType.I32, DataType.I64]:
            is_float = False
        elif data_type in [DataType.F32, DataType.F64]:
            is_float = True
        
        # 优先尝试分配空闲寄存器，但排除s0（帧指针）
        if is_float:
            reg_pool = self.float_regs
        else:
            # 从temp_regs和saved_regs中排除s0
            available_temp_regs = [reg for reg in self.temp_regs if reg != 's0']
            available_saved_regs = [reg for reg in self.saved_regs if reg != 's0']
            reg_pool = available_temp_regs + available_saved_regs
        
        for reg in reg_pool:
            if not self.reg_in_use[reg]:
                self.reg_map[virtual_reg] = reg
                self.reg_in_use[reg] = True
                return reg
        
        # 寄存器不足，溢出到栈
        if is_float:
            size = 4 if data_type == DataType.F32 else 8
        else:
            size = 4  # 默认4字节
        
        if virtual_reg not in self.stack_frame:
            # 为临时变量分配栈空间时，要避免与保留区域冲突
            self.temp_stack_offset += size
            # 确保栈对齐（4字节对齐）
            if self.temp_stack_offset % 4 != 0:
                self.temp_stack_offset = (self.temp_stack_offset + 3) // 4 * 4
            
            # 检查是否会与保留区域冲突
            if hasattr(self, 'reserved_stack_top') and self.temp_stack_offset > self.reserved_stack_top - 32:
                # 如果接近保留区域，调整偏移
                self.temp_stack_offset = max(self.stack_offset + 100, self.temp_stack_offset)
            
            self.stack_frame[virtual_reg] = self.temp_stack_offset
        
        return f"{self.stack_frame[virtual_reg]}(sp)"
    
    def free_register(self, reg):
        """释放物理寄存器"""
        if reg in self.reg_in_use:
            self.reg_in_use[reg] = False
    
    def free_virtual_reg(self, virtual_reg):
        """释放虚拟寄存器占用的资源"""
        if virtual_reg in self.reg_map:
            reg = self.reg_map[virtual_reg]
            self.free_register(reg)
            del self.reg_map[virtual_reg]
    
    def get_stack_size(self):
        """获取栈帧大小"""
        return self.stack_offset
    
    def get_physical_reg(self, virtual_reg):
        """获取虚拟寄存器对应的物理寄存器或栈位置"""
        if virtual_reg in self.reg_map:
            return self.reg_map[virtual_reg]
        if virtual_reg in self.stack_frame:
            return f"{self.stack_frame[virtual_reg]}(sp)"
        return None
    
    def get_temp_register(self):
        """获取一个临时寄存器"""
        # 改进的临时寄存器分配，确保不使用s0（帧指针）
        temp_candidates = ['t0', 't1', 't2', 't3', 't4', 't5', 't6', 's1', 's2', 's3', 's4', 's5', 's6', 's7']
        
        # 使用轮转分配策略
        if not hasattr(self, 'temp_register_counter'):
            self.temp_register_counter = 0
        
        # 尝试找到一个未被使用的寄存器
        for i in range(len(temp_candidates)):
            candidate_idx = (self.temp_register_counter + i) % len(temp_candidates)
            reg = temp_candidates[candidate_idx]
            
            if not self.reg_in_use.get(reg, False):
                self.temp_register_counter = (candidate_idx + 1) % len(temp_candidates)
                # 不要标记为永久使用，这样可以被重复使用
                return reg
        
        # 如果所有寄存器都被使用，使用轮转策略强制分配（但永远不使用s0）
        reg = temp_candidates[self.temp_register_counter % len(temp_candidates)]
        self.temp_register_counter = (self.temp_register_counter + 1) % len(temp_candidates)
        return reg
    
    def get_unique_temp_registers(self, count):
        """获取多个不同的临时寄存器"""
        # 排除s0（帧指针），使用其他临时寄存器
        temp_candidates = ['s1', 's2', 's3', 's4', 's5', 's6', 's7', 't6']
        allocated_regs = []
        
        for i in range(min(count, len(temp_candidates))):
            reg = temp_candidates[i]
            if not self.reg_in_use.get(reg, False):
                self.reg_in_use[reg] = True
                allocated_regs.append(reg)
            else:
                # 如果寄存器被占用，使用备用方案（但永远不使用s0）
                backup_reg = temp_candidates[(i + 1) % len(temp_candidates)]
                allocated_regs.append(backup_reg)
        
        # 如果需要的寄存器数量超过可用数量，重复使用
        while len(allocated_regs) < count:
            allocated_regs.append(temp_candidates[len(allocated_regs) % len(temp_candidates)])
        
        return allocated_regs
    
    def store_to_stack_if_needed(self, result_reg, stack_location, data_type, riscv_instructions):
        """如果目标是栈位置，将寄存器值存储到栈上"""
        if '(sp)' in stack_location:
            if data_type in [DataType.F32, DataType.F64]:
                store_instr = "fsw" if data_type == DataType.F32 else "fsd"
            else:
                store_instr = "sw"
            riscv_instructions.append(f"    {store_instr} {result_reg}, {stack_location}")
            return True
        return False