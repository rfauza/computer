#include "alu_tests.hpp"
#include "../parts/ALU.hpp"
#include "../components/Signal_Generator.hpp"
#include <iostream>
#include <vector>
#include <sstream>

/**
 * @brief Convert array of bools to integer
 */
static int bools_to_int(const ALU* alu, uint16_t num_bits)
{
    int value = 0;
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        if (alu->get_output(i))
        {
            value |= (1 << i);
        }
    }
    return value;
}

/**
 * @brief Get comparison flag from ALU
 */
static bool get_comp_flag(const ALU* alu, uint16_t num_bits, uint16_t flag_idx)
{
    return alu->get_output(num_bits + flag_idx);
}

/**
 * @brief Check expected comparison flags
 * @return true if all flags match expected
 */
static bool check_comp_flags(const ALU* alu, uint16_t num_bits, int a, int b, 
                             bool print_all, const std::string& op_name)
{
    int max_val = (1 << num_bits) - 1;
    
    // Expected flags based on A - B comparison
    bool exp_eq = (a == b);
    bool exp_neq = (a != b);
    bool exp_lt_u = (a < b);  // unsigned comparison
    bool exp_gt_u = (a > b);  // unsigned comparison
    
    // Signed comparison: (disabled)
    // int signed_a = (a > (max_val >> 1)) ? (a - (max_val + 1)) : a;
    // int signed_b = (b > (max_val >> 1)) ? (b - (max_val + 1)) : b;
    // bool exp_lt_s = (signed_a < signed_b);
    // bool exp_gt_s = (signed_a > signed_b);
    
    // Get actual flags
    bool act_eq = get_comp_flag(alu, num_bits, 0);
    bool act_neq = get_comp_flag(alu, num_bits, 1);
    bool act_lt_u = get_comp_flag(alu, num_bits, 2);
    bool act_gt_u = get_comp_flag(alu, num_bits, 3);
    // Signed flags disabled in tests
    // bool act_lt_s = get_comp_flag(alu, num_bits, 4);
    // bool act_gt_s = get_comp_flag(alu, num_bits, 5);
    
    bool all_match = (act_eq == exp_eq) && (act_neq == exp_neq) && 
                    //  (act_lt_s == exp_lt_s) && (act_gt_s == exp_gt_s) &&
                     (act_lt_u == exp_lt_u) && (act_gt_u == exp_gt_u);
    
    if (!all_match || print_all)
    {
        if (!all_match)
        {
            std::cout << "  ✗ FLAGS " << op_name << ": a=" << a << " b=" << b 
                      << " | EQ:" << (act_eq == exp_eq ? "✓" : "✗")
                      << " NEQ:" << (act_neq == exp_neq ? "✓" : "✗")
                      << " LT_U:" << (act_lt_u == exp_lt_u ? "✓" : "✗")
                      << " GT_U:" << (act_gt_u == exp_gt_u ? "✓" : "✗")
                    //   << " LT_S:" << (act_lt_s == exp_lt_s ? "✓" : "✗")
                    //   << " GT_S:" << (act_gt_s == exp_gt_s ? "✓" : "✗")
                      << "\n";
        }
    }
    
    return all_match;
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

static std::string flags_to_string(ALU* alu, uint16_t num_bits)
{
    std::ostringstream ss;
    ss << "EQ=" << get_comp_flag(alu, num_bits, 0)
       << " NEQ=" << get_comp_flag(alu, num_bits, 1)
       << " LT_U=" << get_comp_flag(alu, num_bits, 2)
       << " GT_U=" << get_comp_flag(alu, num_bits, 3)
       << " LT_S=" << get_comp_flag(alu, num_bits, 4)
       << " GT_S=" << get_comp_flag(alu, num_bits, 5);
    return ss.str();
}

void test_alu_truth_table(uint16_t num_bits, bool print_all)
{
    const int max_val = (1 << num_bits) - 1;

    ALU* alu = new ALU(num_bits);
    uint16_t num_inputs = 2 * num_bits + 11;  // data_a + data_b + 11 enables
    std::vector<Signal_Generator> sig_gens(num_inputs);
    
    // Connect all signal generators to ALU inputs
    for (uint16_t i = 0; i < num_inputs; ++i)
        sig_gens[i].connect_output(alu, 0, i);

    int failures = 0;
    int total_tests = 0;
    int flag_failures = 0;

    std::cout << "\n=== Testing ALU Truth Table (num_bits=" << num_bits << ") ===\n";
    
    for (int a = 0; a <= max_val; ++a)
    {
        for (int b = 0; b <= max_val; ++b)
        {
            // Set data A and B
            set_value(sig_gens, 0, a, num_bits);
            set_value(sig_gens, num_bits, b, num_bits);
            
            // Prepare enables: all low
            for (int i = 0; i < 11; ++i) 
                sig_gens[2 * num_bits + i].go_low();

            // --- ADD ---
            sig_gens[2 * num_bits + 0].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            alu->evaluate();
            {
                int actual = bools_to_int(alu, num_bits);
                int expect = (a + b) & max_val;
                bool pass = (actual == expect);
                bool flags_pass = check_comp_flags(alu, num_bits, a, b, print_all, "ADD");
                ++total_tests;
                if (!pass || !flags_pass || print_all)
                {
                    std::cout << (pass ? "✓ " : "✗ ") << "ADD: a=" << a << " b=" << b 
                              << " = " << actual << " (expected " << expect << ")"
                              << " | FLAGS: " << flags_to_string(alu, num_bits) << "\n";
                }
                if (!pass) ++failures;
                if (!flags_pass) { ++failures; ++flag_failures; }
            }
            sig_gens[2 * num_bits + 0].go_low();

            // --- SUB ---
            sig_gens[2 * num_bits + 1].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            alu->evaluate();
            {
                int actual = bools_to_int(alu, num_bits);
                int expect = (a - b) & max_val;
                bool pass = (actual == expect);
                bool flags_pass = check_comp_flags(alu, num_bits, a, b, print_all, "SUB");
                ++total_tests;
                if (!pass || !flags_pass || print_all)
                {
                    std::cout << (pass ? "✓ " : "✗ ") << "SUB: a=" << a << " b=" << b 
                              << " = " << actual << " (expected " << expect << ")"
                              << " | FLAGS: " << flags_to_string(alu, num_bits) << "\n";
                }
                if (!pass) ++failures;
                if (!flags_pass) { ++failures; ++flag_failures; }
            }
            sig_gens[2 * num_bits + 1].go_low();

            // --- INC --- (depends only on A)
            sig_gens[2 * num_bits + 2].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            alu->evaluate();
            {
                int actual = bools_to_int(alu, num_bits);
                int expect = (a + 1) & max_val;
                bool pass = (actual == expect);
                bool flags_pass = check_comp_flags(alu, num_bits, a, b, print_all, "INC");
                ++total_tests;
                if (!pass || !flags_pass || print_all)
                {
                    std::cout << (pass ? "✓ " : "✗ ") << "INC: a=" << a << " b=" << b
                              << " = " << actual << " (expected " << expect << ")"
                              << " | FLAGS: " << flags_to_string(alu, num_bits) << "\n";
                }
                if (!pass) ++failures;
                if (!flags_pass) { ++failures; ++flag_failures; }
            }
            sig_gens[2 * num_bits + 2].go_low();

            // --- DEC --- (depends only on A)
            sig_gens[2 * num_bits + 3].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            alu->evaluate();
            {
                int actual = bools_to_int(alu, num_bits);
                int expect = (a - 1) & max_val;
                bool pass = (actual == expect);
                bool flags_pass = check_comp_flags(alu, num_bits, a, b, print_all, "DEC");
                ++total_tests;
                if (!pass || !flags_pass || print_all)
                {
                    std::cout << (pass ? "✓ " : "✗ ") << "DEC: a=" << a << " b=" << b
                              << " = " << actual << " (expected " << expect << ")"
                              << " | FLAGS: " << flags_to_string(alu, num_bits) << "\n";
                }
                if (!pass) ++failures;
                if (!flags_pass) { ++failures; ++flag_failures; }
            }
            sig_gens[2 * num_bits + 3].go_low();

            // --- MUL ---
            sig_gens[2 * num_bits + 4].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            alu->evaluate();
            {
                int actual = bools_to_int(alu, num_bits);
                int expect = (a * b) & max_val;
                bool pass = (actual == expect);
                bool flags_pass = check_comp_flags(alu, num_bits, a, b, print_all, "MUL");
                ++total_tests;
                if (!pass || !flags_pass || print_all)
                {
                    std::cout << (pass ? "✓ " : "✗ ") << "MUL: a=" << a << " b=" << b 
                              << " = " << actual << " (expected " << expect << ")"
                              << " | FLAGS: " << flags_to_string(alu, num_bits) << "\n";
                }
                if (!pass) ++failures;
                if (!flags_pass) { ++failures; ++flag_failures; }
            }
            sig_gens[2 * num_bits + 4].go_low();

            // --- AND ---
            sig_gens[2 * num_bits + 5].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            alu->evaluate();
            {
                int actual = bools_to_int(alu, num_bits);
                int expect = (a & b) & max_val;
                bool pass = (actual == expect);
                bool flags_pass = check_comp_flags(alu, num_bits, a, b, print_all, "AND");
                ++total_tests;
                if (!pass || !flags_pass || print_all)
                {
                    std::cout << (pass ? "✓ " : "✗ ") << "AND: a=" << a << " b=" << b 
                              << " = " << actual << " (expected " << expect << ")"
                              << " | FLAGS: " << flags_to_string(alu, num_bits) << "\n";
                }
                if (!pass) ++failures;
                if (!flags_pass) { ++failures; ++flag_failures; }
            }
            sig_gens[2 * num_bits + 5].go_low();

            // --- OR ---
            sig_gens[2 * num_bits + 6].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            alu->evaluate();
            {
                int actual = bools_to_int(alu, num_bits);
                int expect = (a | b) & max_val;
                bool pass = (actual == expect);
                bool flags_pass = check_comp_flags(alu, num_bits, a, b, print_all, "OR");
                ++total_tests;
                if (!pass || !flags_pass || print_all)
                {
                    std::cout << (pass ? "✓ " : "✗ ") << "OR: a=" << a << " b=" << b 
                              << " = " << actual << " (expected " << expect << ")"
                              << " | FLAGS: " << flags_to_string(alu, num_bits) << "\n";
                }
                if (!pass) ++failures;
                if (!flags_pass) { ++failures; ++flag_failures; }
            }
            sig_gens[2 * num_bits + 6].go_low();

            // --- XOR ---
            sig_gens[2 * num_bits + 7].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            alu->evaluate();
            {
                int actual = bools_to_int(alu, num_bits);
                int expect = (a ^ b) & max_val;
                bool pass = (actual == expect);
                bool flags_pass = check_comp_flags(alu, num_bits, a, b, print_all, "XOR");
                ++total_tests;
                if (!pass || !flags_pass || print_all)
                {
                    std::cout << (pass ? "✓ " : "✗ ") << "XOR: a=" << a << " b=" << b 
                              << " = " << actual << " (expected " << expect << ")"
                              << " | FLAGS: " << flags_to_string(alu, num_bits) << "\n";
                }
                if (!pass) ++failures;
                if (!flags_pass) { ++failures; ++flag_failures; }
            }
            sig_gens[2 * num_bits + 7].go_low();

            // --- NOT --- (depends only on A)
            sig_gens[2 * num_bits + 8].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            alu->evaluate();
            {
                int actual = bools_to_int(alu, num_bits);
                int expect = (~a) & max_val;
                bool pass = (actual == expect);
                bool flags_pass = check_comp_flags(alu, num_bits, a, b, print_all, "NOT");
                ++total_tests;
                if (!pass || !flags_pass || print_all)
                {
                    std::cout << (pass ? "✓ " : "✗ ") << "NOT: a=" << a << " b=" << b
                              << " = " << actual << " (expected " << expect << ")"
                              << " | FLAGS: " << flags_to_string(alu, num_bits) << "\n";
                }
                if (!pass) ++failures;
                if (!flags_pass) { ++failures; ++flag_failures; }
            }
            sig_gens[2 * num_bits + 8].go_low();

            // --- R_SHIFT --- (depends only on A)
            sig_gens[2 * num_bits + 9].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            alu->evaluate();
            {
                int actual = bools_to_int(alu, num_bits);
                int expect = (a >> 1) & max_val;
                bool pass = (actual == expect);
                bool flags_pass = check_comp_flags(alu, num_bits, a, b, print_all, "R_SHIFT");
                ++total_tests;
                if (!pass || !flags_pass || print_all)
                {
                    std::cout << (pass ? "✓ " : "✗ ") << "R_SHIFT: a=" << a << " b=" << b
                              << " = " << actual << " (expected " << expect << ")"
                              << " | FLAGS: " << flags_to_string(alu, num_bits) << "\n";
                }
                if (!pass) ++failures;
                if (!flags_pass) { ++failures; ++flag_failures; }
            }
            sig_gens[2 * num_bits + 9].go_low();

            // --- L_SHIFT --- (depends only on A)
            sig_gens[2 * num_bits + 10].go_high();
            for (auto &sg : sig_gens) sg.evaluate();
            alu->evaluate();
            {
                int actual = bools_to_int(alu, num_bits);
                int expect = (a << 1) & max_val;
                bool pass = (actual == expect);
                bool flags_pass = check_comp_flags(alu, num_bits, a, b, print_all, "L_SHIFT");
                ++total_tests;
                if (!pass || !flags_pass || print_all)
                {
                    std::cout << (pass ? "✓ " : "✗ ") << "L_SHIFT: a=" << a << " b=" << b
                              << " = " << actual << " (expected " << expect << ")"
                              << " | FLAGS: " << flags_to_string(alu, num_bits) << "\n";
                }
                if (!pass) ++failures;
                if (!flags_pass) { ++failures; ++flag_failures; }
            }
            sig_gens[2 * num_bits + 10].go_low();
        }
    }
    
    std::cout << "\nALU Truth Table: " << total_tests << " tests, ";
    if (failures == 0)
        std::cout << "✓ ALL PASS\n";
    else
        std::cout << "✗ " << failures << " FAILURES (" << flag_failures << " flag failures)\n";

    delete alu;
}
