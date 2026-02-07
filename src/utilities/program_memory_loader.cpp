#include "program_memory_loader.hpp"
#include "../parts/Program_Memory.hpp"
#include "../components/Signal_Generator.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <tuple>

/**
 * @brief Convert binary string to integer
 */
static int binary_to_int(const std::string& binary_str)
{
    int value = 0;
    for (char c : binary_str)
    {
        value = (value << 1) | (c == '1' ? 1 : 0);
    }
    return value;
}

/**
 * @brief Set Program Memory inputs using signal generators
 */
static void set_pm_inputs(
    std::vector<Signal_Generator>& sig_gens,
    Program_Memory& pm,
    int addr, int opcode, int c, int a, int b, bool we, bool re)
{
    uint16_t decoder_bits = pm.get_decoder_bits();
    uint16_t data_bits = pm.get_data_bits();
    
    // Set address bits
    for (uint16_t i = 0; i < decoder_bits; ++i)
    {
        if ((addr >> i) & 1)
            sig_gens[i].go_high();
        else
            sig_gens[i].go_low();
    }
    
    // Set data bits (opcode, C, A, B)
    int data_values[4] = {opcode, c, a, b};
    for (uint16_t reg = 0; reg < 4; ++reg)
    {
        for (uint16_t bit = 0; bit < data_bits; ++bit)
        {
            uint16_t idx = decoder_bits + reg * data_bits + bit;
            if ((data_values[reg] >> bit) & 1)
                sig_gens[idx].go_high();
            else
                sig_gens[idx].go_low();
        }
    }
    
    // Set write enable
    if (we)
        sig_gens[decoder_bits + 4 * data_bits].go_high();
    else
        sig_gens[decoder_bits + 4 * data_bits].go_low();
    
    // Set read enable
    if (re)
        sig_gens[decoder_bits + 4 * data_bits + 1].go_high();
    else
        sig_gens[decoder_bits + 4 * data_bits + 1].go_low();
    
    // Update all signal generators
    for (auto& sg : sig_gens)
    {
        sg.update();
    }
    
    // Evaluate and update PM
    pm.evaluate();
    pm.update();
}

/**
 * @brief Read values from Program Memory outputs
 */
static void read_pm_outputs(
    Program_Memory& pm,
    int& opcode_out, int& c_out, int& a_out, int& b_out)
{
    uint16_t data_bits = pm.get_data_bits();
    
    opcode_out = 0;
    c_out = 0;
    a_out = 0;
    b_out = 0;
    
    // Read opcode
    for (uint16_t i = 0; i < data_bits; ++i)
    {
        if (pm.get_output(i))
            opcode_out |= (1 << i);
    }
    
    // Read C
    for (uint16_t i = 0; i < data_bits; ++i)
    {
        if (pm.get_output(data_bits + i))
            c_out |= (1 << i);
    }
    
    // Read A
    for (uint16_t i = 0; i < data_bits; ++i)
    {
        if (pm.get_output(2 * data_bits + i))
            a_out |= (1 << i);
    }
    
    // Read B
    for (uint16_t i = 0; i < data_bits; ++i)
    {
        if (pm.get_output(3 * data_bits + i))
            b_out |= (1 << i);
    }
}

/**
 * @brief Load program memory from a text file
 * 
 * Parses the file and writes instructions to Program Memory.
 * Modifies expected_data with the loaded instructions for verification.
 * 
 * @param pm Reference to the Program_Memory to load
 * @param filename Path to the text file containing binary program data
 * @param sig_gens Vector of signal generators for PM inputs
 * @param expected_data Reference to vector that will store expected data for verification
 * @return true on success, false on error
 */
static bool load_program_memory_from_file(
    Program_Memory& pm, 
    const std::string& filename,
    std::vector<Signal_Generator>& sig_gens,
    std::vector<std::tuple<int, int, int, int, int>>& expected_data)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file '" << filename << "'\n";
        return false;
    }
    
    uint16_t decoder_bits = pm.get_decoder_bits();
    uint16_t data_bits = pm.get_data_bits();
    
    std::string line;
    int line_number = 0;
    
    std::cout << "=== Loading Program Memory from '" << filename << "' ===\n\n";
    
    // Parse file and write to Program Memory
    while (std::getline(file, line))
    {
        line_number++;
        
        // Trim leading and trailing whitespace (including newlines)
        size_t start = line.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            line.clear();
        } else {
            size_t end = line.find_last_not_of(" \t\n\r");
            line = line.substr(start, end - start + 1);
        }

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';')
        {
            std::cout << "Skipping line " << line_number << ": " << line << "\n";
            continue;
        }
        std::istringstream iss(line);
        std::string addr_str, opcode_str, c_str, a_str, b_str;
        
        if (!(iss >> addr_str >> opcode_str >> c_str >> a_str >> b_str))
        {
            std::cerr << "Error on line " << line_number << ": Invalid format. Expected: pm_addr opcode c a b\n";
            std::cerr << "  Line: " << line << "\n";
            return false;
        }
        
        // Validate binary string lengths
        if (addr_str.length() != static_cast<size_t>(decoder_bits))
        {
            std::cerr << "Error on line " << line_number << ": Address binary string must be exactly " 
                      << decoder_bits << " bits long, got " << addr_str.length() << "\n";
            return false;
        }
        if (opcode_str.length() != static_cast<size_t>(data_bits) ||
            c_str.length() != static_cast<size_t>(data_bits) ||
            a_str.length() != static_cast<size_t>(data_bits) ||
            b_str.length() != static_cast<size_t>(data_bits))
        {
            std::cerr << "Error on line " << line_number << ": Data binary strings must be exactly " 
                      << data_bits << " bits long\n";
            return false;
        }
        
        // Convert binary strings to integers
        int addr = binary_to_int(addr_str);
        int opcode = binary_to_int(opcode_str);
        int c = binary_to_int(c_str);
        int a = binary_to_int(a_str);
        int b = binary_to_int(b_str);
        
        // Validate address range
        int max_addr = (1 << decoder_bits) - 1;
        if (addr < 0 || addr > max_addr)
        {
            std::cerr << "Error on line " << line_number << ": Address " << addr 
                      << " out of range [0, " << max_addr << "]\n";
            return false;
        }
        
        // Validate data range
        int max_data = (1 << data_bits) - 1;
        if (opcode < 0 || opcode > max_data || c < 0 || c > max_data ||
            a < 0 || a > max_data || b < 0 || b > max_data)
        {
            std::cerr << "Error on line " << line_number << ": Data value out of range [0, " 
                      << max_data << "]\n";
            return false;
        }
        
        std::cout << "Writing to address " << addr << ": opcode=" << opcode 
                  << " C=" << c << " A=" << a << " B=" << b << "\n";
        
        // Write to Program Memory (WE=1, RE=0)
        set_pm_inputs(sig_gens, pm, addr, opcode, c, a, b, true, false);
        
        // Store expected data for verification in order
        expected_data.push_back(std::make_tuple(addr, opcode, c, a, b));
    }
    
    std::cout << "\n=== Wrote " << expected_data.size() << " addresses ===\n\n";
    
    return true;
}
bool load_and_verify_program_memory(Program_Memory& pm, const std::string& filename)
{
    uint16_t decoder_bits = pm.get_decoder_bits();
    uint16_t data_bits = pm.get_data_bits();
    uint16_t num_inputs = decoder_bits + (4 * data_bits) + 2;
    
    // Create signal generators
    std::vector<Signal_Generator> sig_gens(num_inputs);
    for (uint16_t i = 0; i < num_inputs; ++i)
    {
        sig_gens[i].connect_output(&pm, 0, i);
    }
    
    // Store expected values for verification in order
    // Vector of tuples: (address, opcode, c, a, b)
    std::vector<std::tuple<int, int, int, int, int>> expected_data;
    
    // Phase 1: Load program memory from file
    if (!load_program_memory_from_file(pm, filename, sig_gens, expected_data))
    {
        return false;  // Loading failed
    }
    
    // Phase 2: Verify by reading back
    std::cout << "=== Verifying Program Memory ===\n\n";
    
    bool verification_passed = true;
    for (const auto& entry : expected_data)
    {
        int addr = std::get<0>(entry);
        int expected_opcode = std::get<1>(entry);
        int expected_c = std::get<2>(entry);
        int expected_a = std::get<3>(entry);
        int expected_b = std::get<4>(entry);
        
        // Read from Program Memory (WE=0, RE=1)
        // Set address and read enable, data inputs don't matter for reading
        set_pm_inputs(sig_gens, pm, addr, 0, 0, 0, 0, false, true);
        
        // Read outputs
        int actual_opcode, actual_c, actual_a, actual_b;
        read_pm_outputs(pm, actual_opcode, actual_c, actual_a, actual_b);
        
        // Verify
        if (actual_opcode == expected_opcode && actual_c == expected_c &&
            actual_a == expected_a && actual_b == expected_b)
        {
            std::cout << "✓ Address " << addr << " verified: opcode=" << actual_opcode 
                      << " C=" << actual_c << " A=" << actual_a << " B=" << actual_b << "\n";
        }
        else
        {
            std::cerr << "✗ Address " << addr << " MISMATCH!\n";
            std::cerr << "  Expected: opcode=" << expected_opcode << " C=" << expected_c 
                      << " A=" << expected_a << " B=" << expected_b << "\n";
            std::cerr << "  Actual:   opcode=" << actual_opcode << " C=" << actual_c 
                      << " A=" << actual_a << " B=" << actual_b << "\n";
            verification_passed = false;
        }
    }
    
    std::cout << "\n=== Verification " << (verification_passed ? "PASSED" : "FAILED") << " ===\n";
    
    return verification_passed;
}
