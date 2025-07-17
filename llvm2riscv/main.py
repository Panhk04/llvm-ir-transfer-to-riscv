#!/usr/bin/env python3
"""
LLVM IR 到 RISC-V 汇编转换器主程序

使用方法:
    python main.py <input_file.ll> <output_file.s>
    
参数:
    input_file.ll   - 输入的 LLVM IR 文件
    output_file.s   - 输出的 RISC-V 汇编文件
"""

import sys
import os
from translator import LLVMIRTranslator

def print_usage():
    """打印使用帮助"""
    print("使用方法:")
    print("    python main.py <input_file.ll> <output_file.s>")
    print()
    print("参数:")
    print("    input_file.ll   - 输入的 LLVM IR 文件")
    print("    output_file.s   - 输出的 RISC-V 汇编文件")
    print()
    print("示例:")
    print("    python main.py test.ll test.s")

def main():
    """主函数"""
    # 检查命令行参数
    if len(sys.argv) != 3:
        print("错误: 参数数量不正确")
        print_usage()
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    # 检查输入文件是否存在
    if not os.path.exists(input_file):
        print(f"错误: 输入文件 '{input_file}' 不存在")
        sys.exit(1)
    
    # 检查输入文件扩展名
    if not input_file.endswith('.ll'):
        print(f"警告: 输入文件 '{input_file}' 不是 .ll 文件")
    
    # 检查输出文件扩展名
    if not output_file.endswith('.s'):
        print(f"警告: 输出文件 '{output_file}' 不是 .s 文件")
    
    try:
        # 读取输入文件
        print(f"正在读取输入文件: {input_file}")
        with open(input_file, 'r', encoding='utf-8') as f:
            ir_code = f.read()
        
        # 检查输入文件是否为空
        if not ir_code.strip():
            print("错误: 输入文件为空")
            sys.exit(1)
        
        # 创建翻译器实例
        print("正在初始化翻译器...")
        translator = LLVMIRTranslator()
        
        # 执行翻译
        print("正在翻译 LLVM IR 到 RISC-V 汇编...")
        try:
            riscv_code = translator.translate(ir_code)
        except Exception as e:
            print(f"错误: 翻译过程中发生错误: {e}")
            sys.exit(1)
        
        # 确保输出目录存在
        output_dir = os.path.dirname(output_file)
        if output_dir and not os.path.exists(output_dir):
            os.makedirs(output_dir)
        
        # 写入输出文件
        print(f"正在写入输出文件: {output_file}")
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(riscv_code)
        
        print("翻译完成!")
        print(f"输入文件: {input_file}")
        print(f"输出文件: {output_file}")
        
        # 显示输出文件大小
        output_size = os.path.getsize(output_file)
        print(f"输出文件大小: {output_size} 字节")
        
    except IOError as e:
        print(f"错误: 文件操作失败: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"错误: 发生未知错误: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()

