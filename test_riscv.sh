#!/bin/bash

# Test script for IR to RISC-V assembly translation

echo "Testing translation to RISC-V assembly..."

# Check if compiler exists
if [ ! -f "build/compiler" ]; then
    echo "Compiler not found. build first."
    # try to build
    echo "Running './build.sh' to build the compiler..."
    ./build.sh
    if [ $? -ne 0 ]; then
        echo "Build failed! Please check the build script."
        exit 1
    fi
    echo "compiler build completed."
fi

# Create test results directory
mkdir -p test_results

# Compile runtime library (shared)
echo "1. Compiling shared runtime library..."
riscv64-linux-gnu-gcc -c runtime.c -o test_results/runtime.o
echo "   Shared runtime library compiled: test_results/runtime.o"
echo

# Function to run detailed test
run_detailed_test() {
    local test_name=$1
    local source_dir="testcases/functional"
    local test_dir="test_results/${test_name}"
    
    # Create test-specific directory
    mkdir -p "$test_dir"
    
    # Copy test files to test directory
    local cact_file="${source_dir}/${test_name}.cact"
    local expected_file="${source_dir}/${test_name}.out"
    local input_file="${source_dir}/${test_name}.in"
    
    if [ ! -f "$cact_file" ]; then
        echo "Error: Test file $cact_file does not exist"
        return 1
    fi
    
    echo "=== Test Case: $test_name ==="
    echo "Test directory: $test_dir"
    
    # Copy source files
    cp "$cact_file" "$test_dir/"
    cp "$expected_file" "$test_dir/"
    if [ -f "$input_file" ]; then
        cp "$input_file" "$test_dir/"
        echo "   ✓ Files copied: ${test_name}.cact, ${test_name}.out, ${test_name}.in"
    else
        echo "   ✓ Files copied: ${test_name}.cact, ${test_name}.out"
    fi
    echo
    
    # Show source code
    echo "2.1 Source code content (${test_name}.cact):"
    echo "----------------------------------------"
    cat "$test_dir/${test_name}.cact"
    echo "----------------------------------------"
    echo
    
    # Show input if exists
    if [ -f "$test_dir/${test_name}.in" ]; then
        echo "2.2 Input data (${test_name}.in):"
        echo "----------------------------------------"
        cat "$test_dir/${test_name}.in"
        echo "----------------------------------------"
        echo
    else
        echo "2.2 No input data file"
        echo
    fi
    
    # Show expected output
    echo "2.3 Expected output (${test_name}.out):"
    echo "----------------------------------------"
    cat "$test_dir/${test_name}.out"
    echo "----------------------------------------"
    echo
    
    # Compile to LLVM IR
    local ir_file="$test_dir/${test_name}.ll"
    echo "2.4 Compiling to LLVM IR..."
    echo "Command: build/compiler -emit-ir $ir_file $test_dir/${test_name}.cact"
    
    if build/compiler -emit-ir "$ir_file" "$test_dir/${test_name}.cact" > "$test_dir/compile.log" 2>&1; then
        echo "   ✓ LLVM IR generated successfully: ${test_name}.ll"
        
        # Show IR size
        local ir_size=$(wc -c < "$ir_file")
        echo "   IR file size: $ir_size bytes"
    else
        echo "   ❌ LLVM IR generation failed"
        echo "   Compilation error log:"
        cat "$test_dir/compile.log"
        return 1
    fi
    
    # Translate to RISC-V assembly using llvm2riscv/translator.py
    local riscv_file="$test_dir/${test_name}.s"
    echo "2.5 Translating to RISC-V assembly..."
    echo "Command: python3 llvm2riscv/main.py $ir_file $riscv_file"
    
    if python3 llvm2riscv/main.py "$ir_file" "$riscv_file" > "$test_dir/translate.log" 2>&1; then
        echo "   ✓ RISC-V assembly generated successfully: ${test_name}.s"
        
        # Show assembly size
        local asm_size=$(wc -c < "$riscv_file")
        echo "   Assembly file size: $asm_size bytes"
        
        # Show first few lines of assembly
        echo "   First 10 lines of assembly:"
        head -10 "$riscv_file" | sed 's/^/     /'
    else
        echo "   ❌ RISC-V assembly generation failed"
        echo "   Translation error log:"
        cat "$test_dir/translate.log"
        return 1
    fi
    
    # Compile RISC-V assembly to executable using riscv64-linux-gnu-gcc
    local exe_file="$test_dir/${test_name}_riscv"
    echo "2.6 Compiling RISC-V assembly to executable..."
    echo "Command: riscv64-linux-gnu-gcc $riscv_file test_results/runtime.o -o $exe_file -static"
    
    if riscv64-linux-gnu-gcc "$riscv_file" test_results/runtime.o -o "$exe_file" -static 2>"$test_dir/link.log"; then
        echo "   ✓ RISC-V executable generated successfully: ${test_name}_riscv"
        
        # Show executable info
        local exe_size=$(wc -c < "$exe_file")
        echo "   Executable size: $exe_size bytes"
    else
        echo "   ❌ RISC-V linking failed"
        echo "   Link error log:"
        cat "$test_dir/link.log"
        return 1
    fi
    
    # Run test using qemu-riscv64
    echo
    echo "2.7 Running RISC-V test with QEMU..."
    local output_file="$test_dir/actual_output.txt"
    local raw_output_file="$test_dir/raw_output.txt"
    
    if [ -f "$test_dir/${test_name}.in" ]; then
        echo "Command: qemu-riscv64 $exe_file < ${test_name}.in"
        timeout 10s qemu-riscv64 "$exe_file" < "$test_dir/${test_name}.in" > "$raw_output_file" 2>&1
        local exit_code=$?
    else
        echo "Command: qemu-riscv64 $exe_file"
        timeout 10s qemu-riscv64 "$exe_file" > "$raw_output_file" 2>&1
        local exit_code=$?
    fi
    
    # Handle timeout
    if [ $exit_code -eq 124 ]; then
        echo "   ⚠️  Test timed out (10 seconds)"
        echo "TIMEOUT" > "$raw_output_file"
        exit_code=1
    fi
    
    # Create combined output (stdout + exit code) like original test script
    cat "$raw_output_file" > "$output_file"
    echo "$exit_code" >> "$output_file"
    
    # Show actual output details
    echo
    echo "2.8 Execution result details:"
    echo "----------------------------------------"
    echo "Standard output:"
    cat "$raw_output_file"
    echo "Exit code: $exit_code"
    echo "----------------------------------------"
    echo "Combined output (for comparison):"
    cat "$output_file" | tr '\n' ' '; echo
    echo "----------------------------------------"
    echo
    
    # Compare results
    echo "2.9 Result comparison:"
    local expected_content=$(cat "$test_dir/${test_name}.out" | tr -d '\n'; echo)
    local actual_content=$(cat "$output_file" | tr -d '\n'; echo)
    
    if [ "$expected_content" = "$actual_content" ]; then
        echo "   ✅ Test passed! Output matches exactly"
        echo "   Expected: '$expected_content'"
        echo "   Actual: '$actual_content'"
        
        # Create success marker
        echo "Test passed" > "$test_dir/TEST_PASSED"
        echo "Expected output: $expected_content" >> "$test_dir/TEST_PASSED"
        echo "Actual output: $actual_content" >> "$test_dir/TEST_PASSED"
        echo "Test time: $(date)" >> "$test_dir/TEST_PASSED"
        
        return 0
    else
        echo "   ❌ Test failed! Output mismatch"
        echo "   Expected: '$expected_content'"
        echo "   Actual: '$actual_content'"
        
        # Create failure report
        echo "Test failed" > "$test_dir/TEST_FAILED"
        echo "Expected output: $expected_content" >> "$test_dir/TEST_FAILED"
        echo "Actual output: $actual_content" >> "$test_dir/TEST_FAILED"
        echo "Test time: $(date)" >> "$test_dir/TEST_FAILED"
        
        # Show diff
        echo
        echo "   Difference details:"
        echo "--- Expected output" > "$test_dir/expected_output.txt"
        echo "$expected_content" >> "$test_dir/expected_output.txt"
        echo "--- Actual output" > "$test_dir/actual_output_labeled.txt"  
        echo "$actual_content" >> "$test_dir/actual_output_labeled.txt"
        diff -u "$test_dir/expected_output.txt" "$test_dir/actual_output_labeled.txt" | tee "$test_dir/diff.txt" || true
        return 1
    fi
}

# Show test directory structure summary
show_test_summary() {
    echo "=== Test Directory Structure ==="
    echo "test_results/"
    echo "├── runtime.o              # Shared runtime library"
    for test_case in "${test_cases[@]}"; do
        local test_dir="test_results/${test_case}"
        if [ -d "$test_dir" ]; then
            echo "├── ${test_case}/"
            echo "│   ├── ${test_case}.cact      # Source code"
            echo "│   ├── ${test_case}.out       # Expected output"
            if [ -f "$test_dir/${test_case}.in" ]; then
                echo "│   ├── ${test_case}.in        # Input data"
            fi
            echo "│   ├── ${test_case}.ll        # Generated LLVM IR"
            echo "│   ├── ${test_case}.s         # Generated RISC-V assembly"
            echo "│   ├── ${test_case}_riscv     # RISC-V executable"
            echo "│   ├── raw_output.txt     # Program raw output"
            echo "│   ├── actual_output.txt  # Combined output(stdout+exitcode)"
            echo "│   ├── compile.log        # Compilation log"
            echo "│   ├── translate.log      # Translation log"
            echo "│   ├── link.log           # Link log"
            if [ -f "$test_dir/TEST_PASSED" ]; then
                echo "│   └── TEST_PASSED        # ✅ Test passed"
            elif [ -f "$test_dir/TEST_FAILED" ]; then
                echo "│   ├── TEST_FAILED        # ❌ Test failed"
                echo "│   └── diff.txt           # Output difference"
            fi
        fi
    done
    echo
}

# Check for required tools
check_tools() {
    echo "Checking required tools..."
    
    local missing_tools=()
    
    if ! command -v python3 &> /dev/null; then
        missing_tools+=("python3")
    fi
    
    if ! command -v riscv64-linux-gnu-gcc &> /dev/null; then
        missing_tools+=("riscv64-linux-gnu-gcc")
    fi
    
    if ! command -v qemu-riscv64 &> /dev/null; then
        missing_tools+=("qemu-riscv64")
    fi
    
    if [ ${#missing_tools[@]} -gt 0 ]; then
        echo "❌ Missing required tools: ${missing_tools[*]}"
        echo "Please install them first:"
        echo "  sudo apt install python3 gcc-riscv64-linux-gnu qemu-user"
        exit 1
    fi
    
    echo "✓ All required tools are available"
    echo
}

# Run tool check
check_tools

# Run detailed tests for selected cases
test_cases=()

# If arguments provided, test specific cases
if [ $# -gt 0 ]; then
    test_cases=("$@")
else
    # Default: test first few cases for demonstration
    test_cases=("00" "01" "02" "03")
fi

echo "Selected test cases: ${test_cases[*]}"
echo

passed=0
failed=0

for test_case in "${test_cases[@]}"; do
    if run_detailed_test "$test_case"; then
        ((passed++))
    else
        ((failed++))
    fi
    echo
    echo "========================================"
    echo
done

echo "=== RISC-V Translation Test Complete ==="
echo "Passed: $passed, Failed: $failed"
echo

# Show test summary
show_test_summary

# Show directory contents
# echo "=== test_results/ Directory Details ==="
# find test_results -type f | sort | while read file; do
#     size=$(wc -c < "$file" 2>/dev/null || echo "0")
#     printf "%-40s %8s bytes\n" "$file" "$size"
# done

if [ $failed -eq 0 ]; then
    echo
    echo "🎉 All tests passed!"
    echo "Complete results for each test case are saved in corresponding test_results/{n}/ directories"
    echo "RISC-V assembly files and executables have been generated successfully"
else
    echo
    echo "⚠️  Some tests failed, please check detailed information in corresponding test directories"
    echo "Failed tests will have TEST_FAILED file and diff.txt difference file"
fi