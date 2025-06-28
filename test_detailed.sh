#!/bin/bash

# Detailed test script for CACT compiler with intermediate results

echo "=== CACT Compiler Detailed Test ==="
echo "This script will show the complete compilation and test process"
echo "Each test case will have its own directory with all related files"
echo

# Create results directory
mkdir -p test_results

# Check if compiler exists
if [ ! -f "build/compiler" ]; then
    echo "Error: Compiler not found, please run ./build.sh first"
    exit 1
fi

# Compile runtime library (shared)
echo "1. Compiling shared runtime library..."
clang -c runtime.c -o test_results/runtime.o
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
        
        # Show compilation log
        if [ -s "$test_dir/compile.log" ]; then
            echo "   Compilation log:"
            cat "$test_dir/compile.log"
        fi
        
        # Show LLVM IR statistics
        local ir_lines=$(wc -l < "$ir_file")
        local ir_size=$(wc -c < "$ir_file")
        echo "   IR statistics: $ir_lines lines, $ir_size bytes"
        
        # Show first 50 lines of LLVM IR
        echo
        echo "2.5 Generated LLVM IR (first 50 lines):"
        echo "----------------------------------------"
        head -50 "$ir_file"
        if [ $ir_lines -gt 50 ]; then
            echo "... (omitted $((ir_lines - 50)) lines)"
        fi
        echo "----------------------------------------"
        echo "   (Complete IR saved in: $ir_file)"
        echo
    else
        echo "   ❌ LLVM IR generation failed"
        echo "   Compilation error log:"
        cat "$test_dir/compile.log"
        return 1
    fi
    
    # Compile to executable
    local exe_file="$test_dir/${test_name}_exe"
    echo "2.6 Linking to executable..."
    echo "Command: clang $ir_file test_results/runtime.o -o $exe_file"
    
    if clang "$ir_file" test_results/runtime.o -o "$exe_file" 2>"$test_dir/link.log"; then
        echo "   ✓ Executable generated successfully: ${test_name}_exe"
        
        # Show executable info
        local exe_size=$(wc -c < "$exe_file")
        echo "   Executable size: $exe_size bytes"
    else
        echo "   ❌ Linking failed"
        echo "   Link error log:"
        cat "$test_dir/link.log"
        return 1
    fi
    
    # Run test
    echo
    echo "2.7 Running test..."
    local output_file="$test_dir/actual_output.txt"
    local raw_output_file="$test_dir/raw_output.txt"
    
    if [ -f "$test_dir/${test_name}.in" ]; then
        echo "Command: ./$exe_file < ${test_name}.in"
        timeout 5s "$exe_file" < "$test_dir/${test_name}.in" > "$raw_output_file" 2>&1
        local exit_code=$?
    else
        echo "Command: ./$exe_file"
        timeout 5s "$exe_file" > "$raw_output_file" 2>&1
        local exit_code=$?
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
            echo "│   ├── ${test_case}_exe       # Executable file"
            echo "│   ├── raw_output.txt     # Program raw output"
            echo "│   ├── actual_output.txt  # Combined output(stdout+exitcode)"
            echo "│   ├── compile.log        # Compilation log"
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

# Run detailed tests for selected cases
test_cases=()

# If arguments provided, test specific cases
if [ $# -gt 0 ]; then
    test_cases=("$@")
else
    # Default: test first few cases for demonstration
    test_cases=("06" "09" "17" "24")
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

echo "=== Detailed Test Complete ==="
echo "Passed: $passed, Failed: $failed"
echo

# Show test summary
show_test_summary

# Show directory contents
echo "=== test_results/ Directory Details ==="
find test_results -type f | sort | while read file; do
    size=$(wc -c < "$file" 2>/dev/null || echo "0")
    printf "%-40s %8s bytes\n" "$file" "$size"
done

if [ $failed -eq 0 ]; then
    echo
    echo "🎉 All tests passed!"
    echo "Complete results for each test case are saved in corresponding test_results/{n}/ directories"
else
    echo
    echo "⚠️  Some tests failed, please check detailed information in corresponding test directories"
    echo "Failed tests will have TEST_FAILED file and diff.txt difference file"
fi 