#include "arithmetic_unit_tests.hpp"
#include "../parts/Arithmetic_Unit.hpp"
#include "../components/Signal_Generator.hpp"
#include <iostream>
#include <vector>

/**
 * @brief Convert array of bools to integer
 */
static int bools_to_int(const Arithmetic_Unit* au, uint16_t num_bits)
{
    int value = 0;
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        if (au->get_output(i))
        {
            value |= (1 << i);
        }
    }
    return value;
}

/**
 * @brief Set signal generators for a value
 */
static void set_value(std::vector<Signal_Generator>& sig_gens, uint16_t start_idx, int value, uint16_t num_bits)
{
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        if ((value >> i) & 1)
            sig_gens[start_idx + i].go_high();
        else
            sig_gens[start_idx + i].go_low();
    }
}

/**
 * @brief Test arithmetic operation
 */
static void test_operation(Arithmetic_Unit* au, std::vector<Signal_Generator>& sig_gens,
                          uint16_t num_bits, int a, int b, int enable_idx, 
                          const std::string& op_name, int expected)
{
    // Set data A
    set_value(sig_gens, 0, a, num_bits);
    
    // Set data B
    set_value(sig_gens, num_bits, b, num_bits);
    
    // Set all enables to low
    for (int i = 0; i < 4; ++i)
    {
        sig_gens[2 * num_bits + i].go_low();
    }
    
    // Set the specific enable high
    sig_gens[2 * num_bits + enable_idx].go_high();
    
    // Update all signal generators
    for (auto& sg : sig_gens)
    {
        sg.update();
    }
    
    // Evaluate arithmetic unit
    au->evaluate();
    
    // Read result
    int result = bools_to_int(au, num_bits);
    
    // Check result
    bool passed = (result == expected);
    
    std::cout << (passed ? "✓" : "✗") << " " << op_name << ": "
              << a << " op " << b << " = " << result
              << " (expected " << expected << ")"
              << (passed ? "" : " FAILED") << "\n";
}

void test_arithmetic_unit()
{
    std::cout << "\n=== Arithmetic Unit Tests ===\n\n";
    
    const uint16_t num_bits = 8;
    const int max_val = (1 << num_bits) - 1; // 255 for 8-bit
    
    // Create arithmetic unit on heap (too large for stack)
    Arithmetic_Unit* au = new Arithmetic_Unit(num_bits);
    
    // Create signal generators
    // Inputs: data_a (num_bits) + data_b (num_bits) + add_enable + sub_enable + inc_enable + dec_enable + mul_enable + div_enable
    uint16_t num_inputs = 2 * num_bits + 6;
    std::vector<Signal_Generator> sig_gens(num_inputs);
    
    // Connect signal generators to arithmetic unit
    for (uint16_t i = 0; i < num_inputs; ++i)
    {
        sig_gens[i].connect_output(au, 0, i);
    }
    
    std::cout << "Testing Addition:\n";
    test_operation(au, sig_gens, num_bits, 5, 3, 0, "ADD", 8);
    test_operation(au, sig_gens, num_bits, 100, 50, 0, "ADD", 150);
    test_operation(au, sig_gens, num_bits, 200, 100, 0, "ADD", (300 & max_val)); // Overflow
    test_operation(au, sig_gens, num_bits, 0, 0, 0, "ADD", 0);
    test_operation(au, sig_gens, num_bits, 127, 128, 0, "ADD", 255);
    
    std::cout << "\nTesting Subtraction:\n";
    test_operation(au, sig_gens, num_bits, 10, 3, 1, "SUB", 7);
    test_operation(au, sig_gens, num_bits, 100, 50, 1, "SUB", 50);
    test_operation(au, sig_gens, num_bits, 5, 10, 1, "SUB", ((-5) & max_val)); // Underflow (wraps around)
    test_operation(au, sig_gens, num_bits, 255, 1, 1, "SUB", 254);
    test_operation(au, sig_gens, num_bits, 50, 50, 1, "SUB", 0);
    
    std::cout << "\nTesting Multiplication:\n";
    test_operation(au, sig_gens, num_bits, 5, 3, 2, "MUL", 15);
    test_operation(au, sig_gens, num_bits, 10, 10, 2, "MUL", 100);
    test_operation(au, sig_gens, num_bits, 20, 10, 2, "MUL", (200 & max_val));
    test_operation(au, sig_gens, num_bits, 0, 100, 2, "MUL", 0);
    test_operation(au, sig_gens, num_bits, 15, 15, 2, "MUL", (225 & max_val));
    
    std::cout << "\nTesting Division:\n";
    test_operation(au, sig_gens, num_bits, 20, 4, 3, "DIV", 5);
    test_operation(au, sig_gens, num_bits, 100, 10, 3, "DIV", 10);
    test_operation(au, sig_gens, num_bits, 15, 4, 3, "DIV", 3); // Integer division
    test_operation(au, sig_gens, num_bits, 255, 5, 3, "DIV", 51);
    test_operation(au, sig_gens, num_bits, 7, 2, 3, "DIV", 3);
    
    std::cout << "\n=== All Arithmetic Unit Tests Complete ===\n";
    
    delete au;
}
