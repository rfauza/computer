#pragma once

#include <string>

class Program_Memory;

/**
 * @brief Load and verify Program Memory from a text file
 * 
 * The text file format is one instruction per line with space-separated binary values:
 *   pm_addr opcode c a b
 * 
 * Example line:
 *   00111010 1001 1100 1111 0010
 *   (address: 58, opcode: 9, C: 12, A: 15, B: 2)
 * 
 * The function will:
 *   1. Parse the file and write each instruction to Program Memory
 *   2. Read back all written addresses and verify the data matches
 * 
 * @param pm Reference to the Program_Memory to load
 * @param filename Path to the text file containing binary program data
 * @return true if loading and verification succeeded, false otherwise
 */
bool load_and_verify_program_memory(Program_Memory& pm, const std::string& filename);
