#include "cpu_tests.hpp"
#include "../parts/CPU.hpp"
#include "../components/Signal_Generator.hpp"
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

/**
 * @brief Convert array of bools to integer
 */
static int bools_to_int(const bool* outputs, uint16_t num_bits)
{
    int value = 0;
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        if (outputs[i])
        {
            value |= (1 << i);
        }
    }
    return value;
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

/**
 * @brief Update all signal generators
 */
static void update_signals(std::vector<Signal_Generator>& sig_gens)
{
    for (auto& sg : sig_gens)
    {
        sg.evaluate();
    }
}

void test_cpu(uint16_t num_bits, bool print_all)
{
    std::cout << "\n=== Testing CPU (num_bits=" << num_bits << ") ===\n";
    
    // Define a simple opcode set
    std::ostringstream opcode_spec;
    opcode_spec << "0000 NOP\n"
                << "0001 ADD\n"
                << "0010 SUB\n"
                << "0011 INC\n"
                << "0100 DEC\n"
                << "0101 MUL\n"
                << "0110 AND\n"
                << "0111 OR\n"
                << "1000 XOR\n"
                << "1001 NOT\n"
                << "1010 RSH\n"
                << "1011 LSH\n"
                << "1100 HALT\n"
                << "1101 JMP\n"
                << "1110 CMP\n"
                << "1111 LOAD\n";
    
    CPU* cpu = new CPU(num_bits, opcode_spec.str(), "test_cpu");
    
    // Keep signal generators alive for the duration of the tests to avoid
    // dangling pointers inside the CPU/control unit that reference their outputs.
    std::vector<Signal_Generator> opcode_sigs(num_bits);
    std::vector<Signal_Generator> data_a_sigs(num_bits);
    std::vector<Signal_Generator> data_b_sigs(num_bits);

    int failures = 0;
    int total_tests = 0;
    
    // === TEST 1: Opcode Parsing ===
    std::cout << "\n--- Test 1: Opcode Parsing ---\n";
    {
        std::vector<std::pair<std::string, int>> expected_mappings = {
            {"NOP", 0b0000},
            {"ADD", 0b0001},
            {"SUB", 0b0010},
            {"INC", 0b0011},
            {"MUL", 0b0101},
            {"HALT", 0b1100},
        };
        
        for (const auto& mapping : expected_mappings)
        {
            int opcode = cpu->get_opcode_for_operation(mapping.first);
            bool pass = (opcode == mapping.second);
            ++total_tests;
            
            if (!pass || print_all)
            {
                std::cout << (pass ? "✓ " : "✗ ") << mapping.first 
                          << " -> " << std::setw(4) << std::setfill('0') << std::hex << opcode
                          << " (expected " << std::setw(4) << mapping.second << ")" << std::dec << "\n";
            }
            if (!pass) ++failures;
        }
    }
    
    // === TEST 2: ALU Operations via Opcode ===
    std::cout << "\n--- Test 2: ALU Operations via Opcode ---\n";
    {
        // Use pre-allocated signal generators for opcode and data inputs
        
        // Connect opcode to CPU's control unit decoder
        const bool** opcode_ptrs = new const bool*[num_bits];
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            opcode_ptrs[i] = &opcode_sigs[i].get_outputs()[0];
        }
        cpu->connect_program_memory(opcode_ptrs, nullptr);  // PM address not needed for this test
        
        // Connect data inputs to CPU's ALU
        const bool** data_a_ptrs = new const bool*[num_bits];
        const bool** data_b_ptrs = new const bool*[num_bits];
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            data_a_ptrs[i] = &data_a_sigs[i].get_outputs()[0];
            data_b_ptrs[i] = &data_b_sigs[i].get_outputs()[0];
        }
        cpu->connect_data_inputs(nullptr, data_a_ptrs, data_b_ptrs);
        
        // Test cases: {opcode_name, A, B, expected_result}
        std::vector<std::tuple<std::string, int, int, int>> test_cases = {
            {"ADD", 3, 5, 8},
            {"SUB", 10, 3, 7},
            {"INC", 5, 0, 6},
            {"DEC", 7, 0, 6},
            {"AND", 0b1100, 0b1010, 0b1000},
            {"OR",  0b1100, 0b0011, 0b1111},
            {"XOR", 0b1100, 0b1010, 0b0110},
            {"NOT", 0b1010, 0, ~0b1010 & ((1 << num_bits) - 1)},
        };
        
        for (const auto& test : test_cases)
        {
            std::string opcode_name = std::get<0>(test);
            int a = std::get<1>(test);
            int b = std::get<2>(test);
            int expected = std::get<3>(test);
            
            // Set opcode
            int opcode = cpu->get_opcode_for_operation(opcode_name);
            set_value(opcode_sigs, 0, opcode, num_bits);
            
            // Set data inputs
            set_value(data_a_sigs, 0, a, num_bits);
            set_value(data_b_sigs, 0, b, num_bits);
            
            // Update all signals
            update_signals(opcode_sigs);
            update_signals(data_a_sigs);
            update_signals(data_b_sigs);
            
            // Evaluate CPU
            cpu->evaluate();
            
            // Check result
            int result = bools_to_int(cpu->get_result_outputs(), num_bits);
            bool pass = (result == expected);
            ++total_tests;
            
            if (!pass || print_all)
            {
                std::cout << (pass ? "✓ " : "✗ ") << opcode_name << ": "
                          << a << " op " << b << " = " << result
                          << " (expected " << expected << ")\n";
            }
            if (!pass) ++failures;
        }
        
        delete[] opcode_ptrs;
        delete[] data_a_ptrs;
        delete[] data_b_ptrs;
    }
    
    // === TEST 3: PC Increment ===
    std::cout << "\n--- Test 3: PC Auto-Increment (via Control Unit) ---\n";
    {
        // The PC should increment on each evaluate cycle
        uint16_t pc_bits = 2 * num_bits;
        int initial_pc = bools_to_int(cpu->get_pc_outputs(), pc_bits);
        
        if (print_all)
        {
            std::cout << "Initial PC: " << initial_pc << "\n";
        }
        
        for (int step = 1; step <= 3; ++step)
        {
            cpu->evaluate();
            int current_pc = bools_to_int(cpu->get_pc_outputs(), pc_bits);
            int expected_pc = (initial_pc + step) & ((1 << pc_bits) - 1);
            
            bool pass = (current_pc == expected_pc);
            ++total_tests;
            
            if (!pass || print_all)
            {
                std::cout << (pass ? "✓ " : "✗ ") << "PC step " << step 
                          << ": PC = " << current_pc << " (expected " << expected_pc << ")\n";
            }
            if (!pass) ++failures;
        }
    }
    
    // === Summary ===
    std::cout << "\nCPU Test Summary: " << total_tests << " tests, ";
    if (failures == 0)
        std::cout << "✓ ALL PASS\n";
    else
        std::cout << "✗ " << failures << " FAILURES\n";
    
    delete cpu;
}
