#pragma once

#include <cstdint>

/**
 * @brief Test Control Unit basic functionality
 * 
 * Tests:
 * - PC initialization and increment
 * - PC jump control
 * - Opcode decoder
 * - Comparator flag storage and clearing
 * - RAM page register
 * - Stack pointer and return address
 * 
 * @param num_bits Bit width for the test (e.g., 4)
 * @param print_all If true, prints every test; if false, prints only failures
 */
void test_control_unit(uint16_t num_bits = 4, bool print_all = false);
