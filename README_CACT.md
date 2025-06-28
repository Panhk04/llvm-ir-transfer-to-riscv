# CACT编译器前端

## 项目简介

本项目是一个CACT语言编译器前端，专门为编译原理课程设计。编译器将CACT源代码编译为LLVM IR中间代码，供学生进行后端开发和优化实现。

### 什么是CACT?

CACT是一个用于教学的类C编程语言，支持基本的数据类型、控制结构和函数。本编译器实现了完整的CACT语言前端，包括词法分析、语法分析、语义分析和LLVM IR代码生成。

## 核心特性

- **🎯 教学导向**：专为编译原理课程设计，代码结构清晰易懂
- **⚡ 轻量高效**：仅50个源文件，396KB代码，编译速度极快
- **🏗️ 模块化设计**：前端(frontend)、中间表示(mir)、工具(util)三大模块
- **✅ 功能完整**：支持CACT语言的所有基本特性
- **🧪 充分测试**：33个功能测试用例，覆盖所有语言特性
- **📊 过程透明**：保留完整编译中间结果，便于学习和调试

## 支持的CACT语言特性

### 数据类型
- `int` - 32位整数
- `float` - 32位浮点数
- `char` - 字符类型
- 一维和多维数组

### 控制结构
- `if-else` 条件语句
- `while` 循环
- `for` 循环
- `break` 和 `continue`

### 函数特性
- 函数定义和调用
- 参数传递
- 返回值
- 递归函数

### 内置函数
- `get_int()` - 读取整数
- `get_char()` - 读取字符
- `get_float()` - 读取浮点数
- `print_int(int)` - 输出整数
- `print_char(char)` - 输出字符
- `print_float(float)` - 输出浮点数

## 快速开始

### 环境要求
- Linux操作系统
- GCC或Clang编译器
- CMake 3.10+
- LLVM工具链（用于运行生成的IR）

### 构建编译器

```bash
# 克隆项目（如果从Git获取）
git clone <项目地址>
cd ucas_compiler

# 一键构建
./build.sh
```

构建成功后，编译器位于 `build/compiler`

### 最小示例：编译运行第一个CACT程序

让我们通过一个最简单的例子来了解完整的编译流程：

#### 1. 查看示例程序
```bash
# 查看最简单的CACT程序
cat testcases/functional/00.cact
```
输出：
```c
int main(){
    return 3;
}
```

#### 2. 编译为LLVM IR
```bash
# 编译CACT源码为LLVM IR
build/compiler -emit-ir hello.ll testcases/functional/00.cact
```

#### 3. 查看生成的LLVM IR
```bash
# 查看生成的中间代码
cat hello.ll
```
输出：
```llvm
declare i32 @get_char()
declare float @get_float()
declare i32 @get_int()
declare void @print_char(i32)
declare void @print_float(float)
declare void @print_int(i32)

define dso_local i32 @main() {
b1:
        ret i32 3
}
```

#### 4. 编译运行时库
```bash
# 编译C运行时库
clang -c runtime.c -o runtime.o
```

#### 5. 链接生成可执行文件
```bash
# 链接IR和运行时库
clang hello.ll runtime.o -o hello
```

#### 6. 运行程序
```bash
# 运行程序,并检查退出状态码
./hello; echo $?
```
输出：
```
3
```

#### 7. 验证结果
```bash
# 查看期望输出
cat testcases/functional/00.out
```
输出：
```
3
```

✅ 编译和运行成功！程序正确返回了退出码3。

## 使用方法

### 基本编译命令
```bash
# 编译CACT程序为LLVM IR
build/compiler -emit-ir output.ll input.cact

# 编译运行时库（一次性操作）
clang -c runtime.c -o runtime.o

# 链接并生成可执行文件
clang output.ll runtime.o -o program

# 运行程序
./program
```

### 处理有输入的程序
```bash
# 如果程序需要输入（如测试用例28）
build/compiler -emit-ir test28.ll testcases/functional/28.cact
clang test28.ll runtime.o -o test28

# 使用输入文件运行
./test28 < testcases/functional/28.in
```

## 批量测试

### 快速测试所有用例
```bash
# 运行所有33个功能测试
./test_all.sh
```

### 详细测试流程
```bash
# 查看详细的编译和执行过程（测试特定用例）
./test_detailed.sh 00 06 28

# 查看所有测试的详细过程
./test_detailed.sh $(ls testcases/functional/*.cact | xargs -I {} basename {} .cact)
```

详细测试会在`test_results/`目录下为每个测试用例创建独立文件夹，包含：
- 源代码文件
- 输入文件（如有）
- 期望输出
- 生成的LLVM IR
- 可执行文件
- 实际输出
- 编译和链接日志

## 测试用例类型

### 基础语法测试
```c
// 测试用例 00: 最简程序
int main(){
    return 3;
}
```

### 变量和运算测试
```c
// 测试用例 06: 常量定义
int main(){
    const int a = 10, b = 5;
    return b;
}
```

### 输入输出测试
```c
// 测试用例 28: I/O交互
int g = 0;
int func(int n) {
    g = g + n;
    print_int(g);
    return g;
}
int main() {
    // 复杂的I/O逻辑...
}
```

## 项目结构

```
ucas_compiler/
├── build/                    # 构建目录
│   └── compiler              # 编译器可执行文件
├── src/                      # 源代码 (24个文件, 196KB)
│   ├── frontend/             # 前端：词法、语法、语义分析
│   ├── mir/                  # 中间表示(MIR)
│   ├── util/                 # 工具类
│   └── Compiler.cpp          # 编译器主程序
├── include/                  # 头文件 (26个文件, 200KB)
│   ├── frontend/
│   ├── mir/
│   └── util/
├── testcases/functional/     # 功能测试用例 (33个测试)
├── runtime.c                 # CACT运行时库
├── build.sh                  # 构建脚本
├── test_all.sh              # 批量测试脚本
├── test_detailed.sh         # 详细测试脚本
├── CMakeLists.txt           # 构建配置
└── README_CACT.md           # 项目文档
```

## 编译器架构

### 编译流程
```
CACT源码 → 词法分析 → 语法分析 → 语义分析 → LLVM IR
```

### 核心模块

#### 1. Frontend（前端）
- **词法分析器**：将源码分解为tokens
- **语法分析器**：构建抽象语法树(AST)
- **语义分析器**：类型检查、作用域分析
- **代码生成器**：将AST转换为LLVM IR

#### 2. MIR（中间表示）
- **基本块**：程序的基本执行单元
- **指令表示**：LLVM IR指令的内部表示
- **符号表**：变量和函数的符号信息

#### 3. Util（工具）
- **管理器**：编译过程管理
- **运行时函数**：内置函数声明和调用

## 运行时库详解

运行时库 `runtime.c` 提供了CACT程序与系统的接口：

```c
// 输入函数
int get_int();        // 从标准输入读取整数
char get_char();      // 从标准输入读取字符
float get_float();    // 从标准输入读取浮点数

// 输出函数
void print_int(int);    // 输出整数到标准输出
void print_char(char);  // 输出字符到标准输出
void print_float(float); // 输出浮点数到标准输出
```

## 测试验证

### 测试覆盖范围
所有33个测试用例验证了以下功能：

- ✅ **基础语法**：变量声明、常量定义、基本运算
- ✅ **控制流**：if-else、while循环、for循环
- ✅ **函数**：函数定义、调用、参数传递、递归
- ✅ **数组**：一维数组、多维数组、数组访问
- ✅ **类型系统**：int、float、char类型及转换
- ✅ **输入输出**：所有6个内置I/O函数
- ✅ **作用域**：全局变量、局部变量、函数参数
- ✅ **表达式**：算术、逻辑、关系运算

### 测试结果示例
```
Testing CACT compiler on functional test cases...
PASS: 00
PASS: 01
PASS: 02
...
PASS: 32

Results: 33 passed, 0 failed out of 33 total tests
All tests passed!
```

## 扩展开发

### 为课程学习准备
本编译器前端为学生提供了完整的基础：

1. **后端开发**：学生可以基于生成的LLVM IR实现目标代码生成
2. **优化实现**：可以在MIR阶段添加各种编译优化
3. **语言扩展**：可以扩展CACT语言特性
4. **调试工具**：可以基于现有框架开发调试和分析工具

### 开发建议
```bash
# 开发时的增量构建
cd build && make

# 清理重新构建
rm -rf build && ./build.sh

# 测试特定功能
./test_detailed.sh 00 06 17  # 测试基础功能
./test_detailed.sh 24 25 28  # 测试复杂功能
```

## 常见问题

### Q: 编译器报错如何调试？
A: 查看编译器的错误输出，通常会指明具体的语法或语义错误位置。

### Q: 生成的LLVM IR如何查看？
A: 使用 `-emit-ir` 参数后，IR文件为文本格式，可以直接用文本编辑器查看。

### Q: 如何添加新的内置函数？
A: 在 `runtime.c` 中实现函数，在 `include/util/FrontendInit.h` 中声明。

### Q: 测试失败如何分析？
A: 使用 `./test_detailed.sh` 查看详细的编译和运行过程，检查中间结果。

## 学习资源

- **编译原理教材**：推荐龙书(Compilers: Principles, Techniques, and Tools)
- **LLVM文档**：https://llvm.org/docs/
- **CACT语言规范**：参考测试用例了解语言特性
- **项目源码**：代码结构清晰，适合逐步学习

---

🎓 **课程提示**：本项目为编译原理课程的前端部分，学生需要在此基础上实现后端代码生成和优化。祝学习愉快！ 