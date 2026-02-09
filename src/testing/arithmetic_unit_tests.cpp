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
 * @brief Debug helper: print AU and device IO for an operation
 */
static void print_io(Arithmetic_Unit* au, const std::string& op_name)
{
    if (!au) return;
    std::cout << "--- IO dump for operation: " << op_name << " ---\n";
    au->print_io();

    // Call device-specific printers
    if (op_name == "ADD" || op_name == "SUB" || op_name == "INC" || op_name == "DEC")
    {
        au->print_adder_inputs();
    }
    else if (op_name == "MUL")
    {
        au->print_multiplier_io();
    }

    std::cout << "--- End IO dump ---\n";
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
    
    // Set all enables to low (5 enable signals: add, sub, inc, dec, mul)
    for (int i = 0; i < 5; ++i)
    {
        sig_gens[2 * num_bits + i].go_low();
    }
    
    // Set the specific enable high
    sig_gens[2 * num_bits + enable_idx].go_high();
    
    // Update all signal generators
    for (auto& sg : sig_gens)
    {
        sg.evaluate();
    }
    
    // // Debug: Print AU inputs
    // std::cout << "  AU inputs: ";
    // au->print_inputs();
    // std::cout << "  ";
    // au->print_adder_inputs();
    
    // Evaluate arithmetic unit
    au->evaluate();
    // Debug: print IO after evaluation
    // print_io(au, op_name);
    
    // // Debug: Print AU outputs
    // std::cout << "  AU outputs: ";
    // au->print_outputs();
    
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
    
    const uint16_t num_bits = 4;
    const int max_val = (1 << num_bits) - 1; // 15 for 4-bit
    
    // Create arithmetic unit on heap (too large for stack)
    Arithmetic_Unit* au = new Arithmetic_Unit(num_bits);
    
    // Create signal generators
    // Inputs: data_a (num_bits) + data_b (num_bits) + add_enable + sub_enable + inc_enable + dec_enable + mul_enable
    uint16_t num_inputs = 2 * num_bits + 5;
    std::vector<Signal_Generator> sig_gens(num_inputs);
    
    // Connect signal generators to arithmetic unit
    for (uint16_t i = 0; i < num_inputs; ++i)
    {
        sig_gens[i].connect_output(au, 0, i);
    }
    
    std::cout << "Testing Addition:\n";
    test_operation(au, sig_gens, num_bits, 5, 3, 0, "ADD", 8);
    test_operation(au, sig_gens, num_bits, 10, 5, 0, "ADD", 15);
    test_operation(au, sig_gens, num_bits, 12, 8, 0, "ADD", (20 & max_val)); // Overflow
    test_operation(au, sig_gens, num_bits, 0, 0, 0, "ADD", 0);
    test_operation(au, sig_gens, num_bits, 15, 1, 0, "ADD", 0); // Overflow wraps to 0
    
    std::cout << "\nTesting Subtraction:\n";
    test_operation(au, sig_gens, num_bits, 10, 3, 1, "SUB", 7);
    test_operation(au, sig_gens, num_bits, 12, 5, 1, "SUB", 7);
    test_operation(au, sig_gens, num_bits, 5, 10, 1, "SUB", ((-5) & max_val)); // Underflow (wraps around)
    test_operation(au, sig_gens, num_bits, 15, 1, 1, "SUB", 14);
    test_operation(au, sig_gens, num_bits, 8, 8, 1, "SUB", 0);
    
    std::cout << "\nTesting Increment (INC):\n";
    // INC uses only operand A; B is ignored
    test_operation(au, sig_gens, num_bits, 0, 0, 2, "INC", 1);
    test_operation(au, sig_gens, num_bits, 1, 0, 2, "INC", 2);
    test_operation(au, sig_gens, num_bits, 14, 0, 2, "INC", 15);
    test_operation(au, sig_gens, num_bits, 15, 0, 2, "INC", 0); // overflow wraps

    std::cout << "\nTesting Decrement (DEC):\n";
    test_operation(au, sig_gens, num_bits, 0, 0, 3, "DEC", max_val); // underflow wraps
    test_operation(au, sig_gens, num_bits, 1, 0, 3, "DEC", 0);
    test_operation(au, sig_gens, num_bits, 5, 0, 3, "DEC", 4);
    test_operation(au, sig_gens, num_bits, 15, 0, 3, "DEC", 14);
    
    std::cout << "\nTesting Multiplication:\n";
    test_operation(au, sig_gens, num_bits, 3, 2, 4, "MUL", 6);
    test_operation(au, sig_gens, num_bits, 4, 3, 4, "MUL", 12);
    test_operation(au, sig_gens, num_bits, 5, 2, 4, "MUL", 10);
    test_operation(au, sig_gens, num_bits, 0, 5, 4, "MUL", 0);
    test_operation(au, sig_gens, num_bits, 3, 4, 4, "MUL", 12);

    
    
    std::cout << "\n=== All Arithmetic Unit Tests Complete ===\n";
    
    delete au;
}

void test_arithmetic_unit_truth_table()
{
    const uint16_t num_bits = 4;
    const int max_val = (1 << num_bits) - 1;

    Arithmetic_Unit* au = new Arithmetic_Unit(num_bits);
    uint16_t num_inputs = 2 * num_bits + 5;
    std::vector<Signal_Generator> sig_gens(num_inputs);
    for (uint16_t i = 0; i < num_inputs; ++i)
        sig_gens[i].connect_output(au, 0, i);

    int failures = 0;

    for (int a = 0; a <= max_val; ++a)
    {
        for (int b = 0; b <= max_val; ++b)
        {
            // Set data A and B
            set_value(sig_gens, 0, a, num_bits);
            set_value(sig_gens, num_bits, b, num_bits);
            
            // prepare enables: all low
            for (int i = 0; i < 5; ++i) sig_gens[2 * num_bits + i].go_low();

            // --- ADD ---
            sig_gens[2 * num_bits + 0].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            au->evaluate();
            {
                int actual = bools_to_int(au, num_bits);
                int expect = (a + b) & max_val;
                if (actual != expect)
                {
                    ++failures;
                    std::cout << "FAIL ADD: a=" << a << " b=" << b << " expected=" << expect << " got=" << actual << "\n";
                }
            }
            sig_gens[2 * num_bits + 0].go_low();

            // --- SUB ---
            sig_gens[2 * num_bits + 1].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            au->evaluate();
            {
                int actual = bools_to_int(au, num_bits);
                int expect = (a - b) & max_val;
                if (actual != expect)
                {
                    ++failures;
                    std::cout << "FAIL SUB: a=" << a << " b=" << b << " expected=" << expect << " got=" << actual << "\n";
                }
            }
            sig_gens[2 * num_bits + 1].go_low();

            // --- INC --- (depends only on A)
            sig_gens[2 * num_bits + 2].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            au->evaluate();
            {
                int actual = bools_to_int(au, num_bits);
                int expect = (a + 1) & max_val;
                if (actual != expect)
                {
                    ++failures;
                    std::cout << "FAIL INC: a=" << a << " expected=" << expect << " got=" << actual << "\n";
                }
            }
            sig_gens[2 * num_bits + 2].go_low();

            // --- DEC --- (depends only on A)
            sig_gens[2 * num_bits + 3].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            au->evaluate();
            {
                int actual = bools_to_int(au, num_bits);
                int expect = (a - 1) & max_val;
                if (actual != expect)
                {
                    ++failures;
                    std::cout << "FAIL DEC: a=" << a << " expected=" << expect << " got=" << actual << "\n";
                }
            }
            sig_gens[2 * num_bits + 3].go_low();

            // --- MUL ---
            sig_gens[2 * num_bits + 4].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            au->evaluate();
            {
                int actual = bools_to_int(au, num_bits);
                int expect = (a * b) & max_val;
                if (actual != expect)
                {
                    ++failures;
                    std::cout << "FAIL MUL: a=" << a << " b=" << b << " expected=" << expect << " got=" << actual << "\n";
                }
            }
            sig_gens[2 * num_bits + 4].go_low();
        }
    }
    
     std::cout << "Arithmetic Unit ";
    if (failures == 0)
        std::cout << "✓ Truth table: PASS\n";
    else
        std::cout << "✗ Truth table: FAIL (" << failures << " mismatches)\n";

    delete au;
}

