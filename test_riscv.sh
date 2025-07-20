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
    local verbose=$2
    
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
    
    # For verbose mode, show detailed information
    if [ $verbose -eq 1 ]; then
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
    else
        # For non-verbose mode, just copy files silently
        cp "$cact_file" "$test_dir/" >/dev/null
        cp "$expected_file" "$test_dir/" >/dev/null
        if [ -f "$input_file" ]; then
            cp "$input_file" "$test_dir/" >/dev/null
        fi
    fi
    
    # Compile to LLVM IR
    local ir_file="$test_dir/${test_name}.ll"
    if [ $verbose -eq 1 ]; then
        echo "2.4 Compiling to LLVM IR..."
        echo "Command: build/compiler -emit-ir $ir_file $test_dir/${test_name}.cact"
    fi
    
    if build/compiler -emit-ir "$ir_file" "$test_dir/${test_name}.cact" > "$test_dir/compile.log" 2>&1; then
        if [ $verbose -eq 1 ]; then
            echo "   ✓ LLVM IR generated successfully: ${test_name}.ll"
            local ir_size=$(wc -c < "$ir_file")
            echo "   IR file size: $ir_size bytes"
        fi
    else
        if [ $verbose -eq 1 ]; then
            echo "   ❌ LLVM IR generation failed"
            echo "   Compilation error log:"
            cat "$test_dir/compile.log"
        else
            echo "   ❌ LLVM IR generation failed"
            echo "   Last 3 lines of compile.log:"
            tail -3 "$test_dir/compile.log" | sed 's/^/        /'
        fi
        return 1
    fi
    
    # Translate to RISC-V assembly
    local riscv_file="$test_dir/${test_name}.s"
    if [ $verbose -eq 1 ]; then
        echo "2.5 Translating to RISC-V assembly..."
        echo "Command: python3 llvm2riscv/main.py $ir_file $riscv_file"
    fi
    
    if python3 llvm2riscv/main.py "$ir_file" "$riscv_file" > "$test_dir/translate.log" 2>&1; then
        if [ $verbose -eq 1 ]; then
            echo "   ✓ RISC-V assembly generated successfully: ${test_name}.s"
            local asm_size=$(wc -c < "$riscv_file")
            echo "   Assembly file size: $asm_size bytes"
            echo "   First 10 lines of assembly:"
            head -10 "$riscv_file" | sed 's/^/     /'
        fi
    else
        if [ $verbose -eq 1 ]; then
            echo "   ❌ RISC-V assembly generation failed"
            echo "   Translation error log:"
            cat "$test_dir/translate.log"
        else
            echo "   ❌ RISC-V assembly generation failed"
            echo "   Last 3 lines of translate.log:"
            tail -3 "$test_dir/translate.log" | sed 's/^/        /'
        fi
        return 1
    fi
    
    # Compile RISC-V assembly to executable
    local exe_file="$test_dir/${test_name}_riscv"
    if [ $verbose -eq 1 ]; then
        echo "2.6 Compiling RISC-V assembly to executable..."
        echo "Command: riscv64-linux-gnu-gcc $riscv_file test_results/runtime.o -o $exe_file -static"
    fi
    
    if riscv64-linux-gnu-gcc "$riscv_file" test_results/runtime.o -o "$exe_file" -static 2>"$test_dir/link.log"; then
        if [ $verbose -eq 1 ]; then
            echo "   ✓ RISC-V executable generated successfully: ${test_name}_riscv"
            local exe_size=$(wc -c < "$exe_file")
            echo "   Executable size: $exe_size bytes"
        fi
    else
        if [ $verbose -eq 1 ]; then
            echo "   ❌ RISC-V linking failed"
            echo "   Link error log:"
            cat "$test_dir/link.log"
        else
            echo "   ❌ RISC-V linking failed"
            echo "   Last 3 lines of link.log:"
            tail -3 "$test_dir/link.log" | sed 's/^/        /'
        fi
        return 1
    fi
    
    # Run test using qemu-riscv64
    if [ $verbose -eq 1 ]; then
        echo
        echo "2.7 Running RISC-V test with QEMU..."
    fi
    
    local output_file="$test_dir/actual_output.txt"
    local raw_output_file="$test_dir/raw_output.txt"
    
    if [ -f "$test_dir/${test_name}.in" ]; then
        if [ $verbose -eq 1 ]; then
            echo "Command: qemu-riscv64 $exe_file < ${test_name}.in"
        fi
        timeout 10s qemu-riscv64 "$exe_file" < "$test_dir/${test_name}.in" > "$raw_output_file" 2>&1
        local exit_code=$?
    else
        if [ $verbose -eq 1 ]; then
            echo "Command: qemu-riscv64 $exe_file"
        fi
        timeout 10s qemu-riscv64 "$exe_file" > "$raw_output_file" 2>&1
        local exit_code=$?
    fi
    
    # Handle timeout
    if [ $exit_code -eq 124 ]; then
        if [ $verbose -eq 1 ]; then
            echo "   ⚠️  Test timed out (10 seconds)"
        fi
        echo "TIMEOUT" > "$raw_output_file"
        exit_code=1
    fi
    
    # Create combined output (stdout + exit code)
    cat "$raw_output_file" > "$output_file"
    echo "$exit_code" >> "$output_file"
    
    # For verbose mode, show actual output details
    if [ $verbose -eq 1 ]; then
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
    fi
    
    # Compare results
    if [ $verbose -eq 1 ]; then
        echo "2.9 Result comparison:"
    fi
    
    local expected_content=$(cat "$test_dir/${test_name}.out" | tr -d '\n'; echo)
    local actual_content=$(cat "$output_file" | tr -d '\n'; echo)
    
    if [ "$expected_content" = "$actual_content" ]; then
        if [ $verbose -eq 1 ]; then
            echo "   ✅ Test passed! Output matches exactly"
            echo "   Expected: '$expected_content'"
            echo "   Actual: '$actual_content'"
        fi
        
        # Create success marker
        echo "Test passed" > "$test_dir/TEST_PASSED"
        echo "Expected output: $expected_content" >> "$test_dir/TEST_PASSED"
        echo "Actual output: $actual_content" >> "$test_dir/TEST_PASSED"
        echo "Test time: $(date)" >> "$test_dir/TEST_PASSED"
        
        return 0
    else
        if [ $verbose -eq 1 ]; then
            echo "   ❌ Test failed! Output mismatch"
            echo "   Expected: '$expected_content'"
            echo "   Actual: '$actual_content'"
            
            # Show diff
            echo
            echo "   Difference details:"
            echo "--- Expected output" > "$test_dir/expected_output.txt"
            echo "$expected_content" >> "$test_dir/expected_output.txt"
            echo "--- Actual output" > "$test_dir/actual_output_labeled.txt"  
            echo "$actual_content" >> "$test_dir/actual_output_labeled.txt"
            diff -u "$test_dir/expected_output.txt" "$test_dir/actual_output_labeled.txt" | tee "$test_dir/diff.txt" || true
        else
            # For non-verbose mode, show concise error info
            echo "   ❌ Output mismatch"
            echo "   Expected: ${expected_content:0:50}$([ ${#expected_content} -gt 50 ] && echo "...")"
            echo "   Actual:   ${actual_content:0:50}$([ ${#actual_content} -gt 50 ] && echo "...")"
            
            # Check if timeout occurred
            if grep -q "TIMEOUT" "$raw_output_file"; then
                echo "   ⚠️  Test timed out (10 seconds)"
            fi
            
            # Show exit code comparison
            local expected_exit=$(tail -n 1 "$test_dir/${test_name}.out" 2>/dev/null)
            local actual_exit=$exit_code
            if [[ "$expected_exit" =~ ^[0-9]+$ ]] && [ "$expected_exit" -ne "$actual_exit" ]; then
                echo "   Exit code mismatch: expected $expected_exit, got $actual_exit"
            fi
        fi
        
        # Create failure report
        echo "Test failed" > "$test_dir/TEST_FAILED"
        echo "Expected output: $expected_content" >> "$test_dir/TEST_FAILED"
        echo "Actual output: $actual_content" >> "$test_dir/TEST_FAILED"
        echo "Test time: $(date)" >> "$test_dir/TEST_FAILED"
        
        return 1
    fi
}

# Show test directory structure summary
show_test_summary() {
    echo "=== Test Directory Structure Summary ==="
    echo "test_results/"
    echo "├── runtime.o              # Shared runtime library"
    for test_case in "${test_cases[@]}"; do
        local test_dir="test_results/${test_case}"
        if [ -d "$test_dir" ]; then
            status="❌"
            if [ -f "$test_dir/TEST_PASSED" ]; then
                status="✅"
            fi
            echo "├── ${test_case}/ $status"
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

# Define all test cases (00 to 32)
all_test_cases=($(seq -f "%02g" 0 32))

# Determine verbosity based on number of test cases
if [ $# -gt 0 ]; then
    test_cases=("$@")
    verbose_mode=1  # Detailed output for specific tests
    echo "Running specific test cases: ${test_cases[*]}"
else
    test_cases=("${all_test_cases[@]}")
    verbose_mode=0  # Simplified output for all tests
    echo "Running all ${#test_cases[@]} test cases (00-32)"
fi

total_tests=${#test_cases[@]}
current_test=1
passed=0
failed=0

echo
echo "Starting RISC-V translation tests..."
echo "========================================"
echo

# Run tests for selected cases
for test_case in "${test_cases[@]}"; do
    test_dir="test_results/${test_case}"
    mkdir -p "$test_dir"
    
    if [ $verbose_mode -eq 1 ] || [ $total_tests -eq 1 ]; then
        # Detailed output for specific tests or when testing a single test
        echo ">>> [Test $current_test/$total_tests] Starting test: $test_case"
        echo "------------------------------------------------------------"
        if run_detailed_test "$test_case" 1; then
            echo "✅ [Test $current_test/$total_tests] PASSED: $test_case"
            ((passed++))
        else
            echo "❌ [Test $current_test/$total_tests] FAILED: $test_case"
            ((failed++))
        fi
        echo "------------------------------------------------------------"
    else
        # Simplified output for all tests
        printf "[%02d/%02d] Testing %s: " $current_test $total_tests $test_case
        if run_detailed_test "$test_case" 0 > "$test_dir/run.log" 2>&1; then
            echo "PASSED ✅"
            ((passed++))
        else
            echo "FAILED ❌"
            ((failed++))
            # Show concise error information (3-5 lines)
            echo "   --- Error Summary ---"
            tail -5 "$test_dir/run.log" | grep -v '^+' | head -5 | sed 's/^/   /'
        fi
    fi
    
    current_test=$((current_test + 1))
done

echo "========================================"
echo "=== RISC-V Translation Test Complete ==="
echo "========================================"
echo "Total tests: $total_tests"
echo "Passed: $passed"
echo "Failed: $failed"
echo

# Show test summary
show_test_summary

# Final status message
if [ $failed -eq 0 ]; then
    echo "🎉 All tests passed!"
    echo "Complete results saved in test_results directory"
else
    echo "⚠️  $failed tests failed, check logs in:"
    find test_results -name TEST_FAILED | sed 's/\/TEST_FAILED$//' | xargs -I{} echo "   - {}"
fi

# Exit with appropriate status
exit $failed