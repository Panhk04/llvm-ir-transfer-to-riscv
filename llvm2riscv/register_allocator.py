from collections import defaultdict

class RegisterAllocator:
    def __init__(self):
        # 定义 RISC-V 通用寄存器
        self.available_registers = [f"t{i}" for i in range(7)] + [f"s{i}" for i in range(12)]
        self.register_map = {}
        self.next_register_index = 0

    def allocate_register(self, virtual_register):
        """为虚拟寄存器分配一个真实的 RISC-V 寄存器"""
        if virtual_register in self.register_map:
            return self.register_map[virtual_register]

        if self.next_register_index >= len(self.available_registers):
            raise Exception("No available registers")

        real_register = self.available_registers[self.next_register_index]
        self.register_map[virtual_register] = real_register
        self.next_register_index += 1
        return real_register

    def get_register(self, virtual_register):
        """获取虚拟寄存器对应的真实寄存器"""
        return self.register_map.get(virtual_register)

    def reset(self):
        """重置寄存器分配器"""
        self.register_map = {}
        self.next_register_index = 0

# 测试代码
if __name__ == "__main__":
    allocator = RegisterAllocator()
    virtual_reg1 = "%a"
    virtual_reg2 = "%b"

    real_reg1 = allocator.allocate_register(virtual_reg1)
    real_reg2 = allocator.allocate_register(virtual_reg2)

    print(f"Virtual register {virtual_reg1} is allocated to {real_reg1}")
    print(f"Virtual register {virtual_reg2} is allocated to {real_reg2}")