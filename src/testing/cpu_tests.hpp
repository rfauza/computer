#pragma once

#include <cstdint>

/**
 * @brief Test CPU basic functionality
 * 
 * Tests:
 * - Opcode parsing and mapping
 * - Control Unit and ALU integration
 * - Decoder to ALU enable wiring
 * - PC increment and instruction execution
 * - Various ALU operations (ADD, SUB, INC, etc.)
 * 
 * @param num_bits Bit width for the test (e.g., 4)
 * @param print_all If true, prints every test; if false, prints only failures
 */
void test_cpu(uint16_t num_bits = 4, bool print_all = false);
