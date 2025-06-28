#!/bin/bash

# Test script for CACT compiler

echo "Testing CACT compiler on functional test cases..."

# Check if compiler exists
if [ ! -f "build/compiler" ]; then
    echo "Error: Compiler not found. Please run './build.sh' first."
    exit 1
fi

# Compile runtime library
clang -c runtime.c -o runtime.o

passed=0
failed=0
total=0

# Test all .cact files in functional directory
for cact_file in testcases/functional/*.cact; do
    if [ -f "$cact_file" ]; then
        # Extract test case name (e.g., "00" from "00.cact")
        test_name=$(basename "$cact_file" .cact)
        
        # Check if expected output file exists
        expected_file="testcases/functional/${test_name}.out"
        if [ ! -f "$expected_file" ]; then
            echo "Warning: No expected output file for $test_name"
            continue
        fi
        
        # Compile to LLVM IR
        ir_file="test_${test_name}.ll"
        build/compiler -emit-ir "$ir_file" "$cact_file"
        
        if [ $? -ne 0 ]; then
            echo "FAIL: $test_name - Compilation failed"
            ((failed++))
            ((total++))
            continue
        fi
        
        # Compile to executable
        exe_file="test_${test_name}"
        clang "$ir_file" runtime.o -o "$exe_file" 2>/dev/null
        
        if [ $? -ne 0 ]; then
            echo "FAIL: $test_name - Linking failed"
            ((failed++))
            ((total++))
            continue
        fi
        
        # Run test
        input_file="testcases/functional/${test_name}.in"
        if [ -f "$input_file" ]; then
            # Has input file
            actual_output=$(timeout 5s ./"$exe_file" < "$input_file" 2>/dev/null; echo $?)
        else
            # No input file
            actual_output=$(timeout 5s ./"$exe_file" 2>/dev/null; echo $?)
        fi
        
        # Get expected output
        expected_output=$(cat "$expected_file" | tr -d '\n'; echo)
        
        # Compare outputs
        if [ "$actual_output" = "$expected_output" ]; then
            echo "PASS: $test_name"
            ((passed++))
        else
            echo "FAIL: $test_name - Expected: '$expected_output', Got: '$actual_output'"
            ((failed++))
        fi
        
        ((total++))
        
        # Clean up
        rm -f "$ir_file" "$exe_file"
    fi
done

echo
echo "Results: $passed passed, $failed failed out of $total total tests"

# Clean up runtime object
rm -f runtime.o

if [ $failed -eq 0 ]; then
    echo "All tests passed!"
    exit 0
else
    echo "Some tests failed."
    exit 1
fi 