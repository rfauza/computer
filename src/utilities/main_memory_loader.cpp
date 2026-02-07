#include "main_memory_loader.hpp"
#include "../parts/Main_Memory.hpp"
#include "../components/Signal_Generator.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

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
 * @brief Set Main Memory inputs using signal generators
 */
static void set_mm_inputs(
    std::vector<Signal_Generator>& sig_gens,
    Main_Memory& mm,
    int addr, int data, bool we, bool re);

/**
 * @brief Read value from Main Memory outputs
 */
static int read_mm_output(Main_Memory& mm)
{
    uint16_t data_bits = mm.get_data_bits();
    int data_out = 0;
    
    // Read data
    for (uint16_t i = 0; i < data_bits; ++i)
    {
        if (mm.get_output(i))
            data_out |= (1 << i);
    }
    
    return data_out;
}

/**
 * @brief Load main memory from file
 */
static bool load_main_memory_from_file(
    Main_Memory& mm, 
    const std::string& filename,
    std::vector<Signal_Generator>& sig_gens,
    std::map<int, int>& memory_data)
{
    // Open file
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file: " << filename << "\n";
        return false;
    }
    
    uint16_t address_bits = mm.get_address_bits();
    uint16_t data_bits = mm.get_data_bits();
    
    std::string line;
    int line_num = 0;
    
    std::cout << "=== Loading Main Memory from '" << filename << "' ===\n\n";
    
    // Parse file
    while (std::getline(file, line))
    {
        line_num++;
        
        // Trim leading and trailing whitespace (including newlines)
        size_t start = line.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            line.clear();
        } else {
            size_t end = line.find_last_not_of(" \t\n\r");
            line = line.substr(start, end - start + 1);
        }

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#')
        {
            std::cout << "Skipping line " << line_num << ": " << line << "\n";
            continue;
        }
        
        std::istringstream iss(line);
        std::string addr_str, data_str;
        
        if (!(iss >> addr_str >> data_str))
        {
            std::cerr << "Error on line " << line_num << ": Invalid format. Expected: addr data\n";
            std::cerr << "  Line: " << line << "\n";
            return false;
        }
        
        // Validate binary strings
        if (addr_str.size() != address_bits)
        {
            std::cerr << "Error on line " << line_num << ": Address has " << addr_str.size() 
                      << " bits, expected " << address_bits << "\n";
            return false;
        }
        
        if (data_str.size() != data_bits)
        {
            std::cerr << "Error on line " << line_num << ": Data has " << data_str.size() 
                      << " bits, expected " << data_bits << "\n";
            return false;
        }
        
        // Convert binary strings to integers
        int addr = binary_to_int(addr_str);
        int data = binary_to_int(data_str);
        
        // Validate address range
        int max_addr = (1 << address_bits) - 1;
        if (addr < 0 || addr > max_addr)
        {
            std::cerr << "Error on line " << line_num << ": Address " << addr 
                      << " out of range [0, " << max_addr << "]\n";
            return false;
        }
        
        // Validate data range
        int max_data = (1 << data_bits) - 1;
        if (data < 0 || data > max_data)
        {
            std::cerr << "Error on line " << line_num << ": Data " << data 
                      << " out of range [0, " << max_data << "]\n";
            return false;
        }
        
        std::cout << "Writing to address " << addr << ": data=" << data << "\n";
        
        // Write to Main Memory (WE=1, RE=0)
        set_mm_inputs(sig_gens, mm, addr, data, true, false);
        
        // Store expected data for verification
        memory_data[addr] = data;
    }
    
    std::cout << "\n=== Wrote " << memory_data.size() << " addresses ===\n\n";
    
    return true;
}

/**
 * @brief Set Main Memory inputs using signal generators
 */
static void set_mm_inputs(
    std::vector<Signal_Generator>& sig_gens,
    Main_Memory& mm,
    int addr, int data, bool we, bool re)
{
    uint16_t address_bits = mm.get_address_bits();
    uint16_t data_bits = mm.get_data_bits();
    
    // Set address bits
    for (uint16_t i = 0; i < address_bits; ++i)
    {
        if ((addr >> i) & 1)
            sig_gens[i].go_high();
        else
            sig_gens[i].go_low();
    }
    
    // Set data bits
    for (uint16_t bit = 0; bit < data_bits; ++bit)
    {
        uint16_t idx = address_bits + bit;
        if ((data >> bit) & 1)
            sig_gens[idx].go_high();
        else
            sig_gens[idx].go_low();
    }
    
    // Set write enable
    if (we)
        sig_gens[address_bits + data_bits].go_high();
    else
        sig_gens[address_bits + data_bits].go_low();
    
    // Set read enable
    if (re)
        sig_gens[address_bits + data_bits + 1].go_high();
    else
        sig_gens[address_bits + data_bits + 1].go_low();
    
    // Update all signal generators
    for (auto& sg : sig_gens)
    {
        sg.update();
    }
    
    // Evaluate and update MM
    mm.evaluate();
    mm.update();
}



bool load_and_verify_main_memory(Main_Memory& mm, const std::string& filename)
{
    uint16_t address_bits = mm.get_address_bits();
    uint16_t data_bits = mm.get_data_bits();
    uint16_t num_inputs = address_bits + data_bits + 2;
    
    // Create static signal generators that persist across calls
    static std::vector<Signal_Generator> sig_gens;
    
    // Initialize on first call
    if (sig_gens.empty())
    {
        sig_gens.resize(num_inputs);
        for (uint16_t i = 0; i < num_inputs; ++i)
        {
            sig_gens[i].connect_output(&mm, 0, i);
        }
    }
    
    // Store expected values for verification: address -> data
    std::map<int, int> expected_data;
    
    // Phase 1: Load main memory from file
    if (!load_main_memory_from_file(mm, filename, sig_gens, expected_data))
    {
        return false;  // Loading failed
    }
    
    // Phase 2: Verify by reading back
    std::cout << "=== Verifying Main Memory ===\n\n";
    
    bool verification_passed = true;
    for (const auto& entry : expected_data)
    {
        int addr = entry.first;
        int expected_data_value = entry.second;
        
        // Read from Main Memory (WE=0, RE=1)
        // Set address and read enable, data inputs don't matter for reading
        set_mm_inputs(sig_gens, mm, addr, 0, false, true);
        
        // Read output
        int actual_data = read_mm_output(mm);
        
        if (actual_data == expected_data_value)
        {
            std::cout << "✓ Address " << addr << " verified: data=" << actual_data << "\n";
        }
        else
        {
            std::cerr << "✗ Address " << addr << " MISMATCH!\n";
            std::cerr << "  Expected: data=" << expected_data_value << "\n";
            std::cerr << "  Actual:   data=" << actual_data << "\n";
            verification_passed = false;
        }
    }
    
    std::cout << "\n=== Verification " << (verification_passed ? "PASSED" : "FAILED") << " ===\n";
    
    return verification_passed;
}
