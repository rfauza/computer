#pragma once

#include <string>

class Program_Memory;

/**
 * @brief Test Program Memory with a specific input string
 * 
 * Input format (space-separated):
 *   addr opcode C A B WE RE
 * 
 * @param pm Reference to the Program_Memory to test
 * @param input_str Input specification string
 */
void program_memory_tester(Program_Memory& pm, const std::string& input_str);
