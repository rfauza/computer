#pragma once

#include <string>

class Main_Memory;

/**
 * @brief Test Main Memory with a specific input string
 * 
 * Input format (space-separated):
 *   addr data WE RE
 * 
 * @param mm Reference to the Main_Memory to test
 * @param input_str Input specification string
 */
void main_memory_tester(Main_Memory& mm, const std::string& input_str);
