#pragma once

#include <cstdint>


/**
 * @brief Test all ALU operations with full truth table
 * 
 * Tests all 11 operations (add, sub, inc, dec, mul, and, or, xor, not, r_shift, l_shift)
 * for all input combinations. Prints only failures by default.
 * 
 * @param num_bits Bit width for the test (e.g., 4)
 * @param print_all If true, prints every test; if false, prints only failures
 */
void test_alu_truth_table(uint16_t num_bits, bool print_all=false);
