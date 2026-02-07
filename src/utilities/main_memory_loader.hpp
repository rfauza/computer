#pragma once

#include <string>

class Main_Memory;

/**
 * @brief Load and verify Main Memory from a text file
 * 
 * The text file format is one data value per line with space-separated binary values:
 *   mm_addr data
 * 
 * Example line:
 *   00111010 1001110011110010
 *   (address: 58, data: 40178)
 * 
 * The function will:
 *   1. Parse the file and write each value to Main Memory
 *   2. Read back all written addresses and verify the data matches
 * 
 * @param mm Reference to the Main_Memory to load
 * @param filename Path to the text file containing binary data
 * @return true if loading and verification succeeded, false otherwise
 */
bool load_and_verify_main_memory(Main_Memory& mm, const std::string& filename);
