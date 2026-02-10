#include "control_unit_tests.hpp"
#include "../parts/Control_Unit.hpp"
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
 * @brief Print current state of Control Unit
 */
static void print_cu_state(Control_Unit* cu, uint16_t pc_bits, const std::string& label)
{
    std::cout << label << ":\n";
    std::cout << "  PC = " << std::dec << bools_to_int(cu->get_pc_outputs(), pc_bits) << " (0x" 
              << std::hex << std::setw(pc_bits/4) << std::setfill('0') 
              << bools_to_int(cu->get_pc_outputs(), pc_bits) << ")\n" << std::dec;
    std::cout << "  Flags [EQ,NEQ,LT_U,GT_U,LT_S,GT_S] = [";
    for (int i = 0; i < 6; ++i)
    {
        std::cout << cu->get_stored_flags()[i];
        if (i < 5) std::cout << ",";
    }
    std::cout << "]\n";
    std::cout << "  RAM Page = " << bools_to_int(cu->get_ram_page_outputs(), pc_bits) << "\n";
    std::cout << "  Stack Pointer = " << bools_to_int(cu->get_stack_pointer_outputs(), pc_bits) << "\n";
}

void test_control_unit(uint16_t num_bits, bool print_all)
{
    std::cout << "\n=== Testing Control Unit (num_bits=" << num_bits << ") ===\n";
    
    Control_Unit* cu = new Control_Unit(num_bits);
    uint16_t pc_bits = static_cast<uint16_t>(2 * num_bits);
    
    int failures = 0;
    int total_tests = 0;
    
    // Create signal generators for inputs
    std::vector<Signal_Generator> jump_address_sigs(pc_bits);
    Signal_Generator jump_enable_sig;
    std::vector<Signal_Generator> opcode_sigs(num_bits);
    std::vector<Signal_Generator> flag_sigs(6);  // 6 comparator flags
    std::vector<Signal_Generator> page_data_sigs(pc_bits);
    Signal_Generator page_write_enable_sig;
    
    // Set up connections
    // Jump address
    const bool** jump_addr_ptrs = new const bool*[pc_bits];
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        jump_addr_ptrs[i] = &jump_address_sigs[i].get_outputs()[0];
    }
    cu->connect_jump_address_to_pc(jump_addr_ptrs, pc_bits);
    cu->connect_jump_enable(&jump_enable_sig.get_outputs()[0]);
    
    // Opcode
    const bool** opcode_ptrs = new const bool*[num_bits];
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        opcode_ptrs[i] = &opcode_sigs[i].get_outputs()[0];
    }
    cu->connect_opcode_input(opcode_ptrs, num_bits);
    
    // Flags
    const bool** flag_ptrs = new const bool*[6];
    for (uint16_t i = 0; i < 6; ++i)
    {
        flag_ptrs[i] = &flag_sigs[i].get_outputs()[0];
    }
    cu->connect_comparator_flags(flag_ptrs, 6);
    
    // RAM Page
    const bool** page_ptrs = new const bool*[pc_bits];
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        page_ptrs[i] = &page_data_sigs[i].get_outputs()[0];
    }
    cu->connect_ram_page_data_input(page_ptrs, pc_bits);
    cu->connect_ram_page_write_enable(&page_write_enable_sig.get_outputs()[0]);
    
    // === TEST 1: PC Increment ===
    std::cout << "\n--- Test 1: PC Auto-Increment ---\n";
    {
        // Initialize: jump_enable = 0 (so PC increments)
        jump_enable_sig.go_low();
        jump_enable_sig.evaluate();
        
        // Set all other inputs to 0
        for (auto& sig : jump_address_sigs) { sig.go_low(); sig.evaluate(); }
        for (auto& sig : opcode_sigs) { sig.go_low(); sig.evaluate(); }
        for (auto& sig : flag_sigs) { sig.go_low(); sig.evaluate(); }
        for (auto& sig : page_data_sigs) { sig.go_low(); sig.evaluate(); }
        page_write_enable_sig.go_low();
        page_write_enable_sig.evaluate();
        
        // Initial evaluation
        cu->evaluate();
        int initial_pc = bools_to_int(cu->get_pc_outputs(), pc_bits);
        
        if (print_all)
        {
            print_cu_state(cu, pc_bits, "Initial state");
        }
        
        // Simulate several increments
        for (int step = 1; step <= 5; ++step)
        {
            cu->evaluate();
            int current_pc = bools_to_int(cu->get_pc_outputs(), pc_bits);
            int expected_pc = (initial_pc + step) & ((1 << pc_bits) - 1);
            
            bool pass = (current_pc == expected_pc);
            ++total_tests;
            
            if (!pass || print_all)
            {
                std::cout << (pass ? "✓ " : "✗ ") << "PC Increment step " << step 
                          << ": PC = " << current_pc << " (expected " << expected_pc << ")\n";
            }
            if (!pass) ++failures;
        }
    }
    
    // === TEST 2: PC Jump ===
    std::cout << "\n--- Test 2: PC Jump Control ---\n";
    {
        // Set jump address to a specific value (e.g., 42)
        int jump_target = 42 % (1 << pc_bits);
        set_value(jump_address_sigs, 0, jump_target, pc_bits);
        for (auto& sig : jump_address_sigs) sig.evaluate();
        
        // Enable jump
        jump_enable_sig.go_high();
        jump_enable_sig.evaluate();
        
        cu->evaluate();
        int current_pc = bools_to_int(cu->get_pc_outputs(), pc_bits);
        bool pass = (current_pc == jump_target);
        ++total_tests;
        
        if (!pass || print_all)
        {
            std::cout << (pass ? "✓ " : "✗ ") << "PC Jump: PC = " << current_pc 
                      << " (expected " << jump_target << ")\n";
        }
        if (!pass) ++failures;
        
        // Disable jump and verify increment resumes
        jump_enable_sig.go_low();
        jump_enable_sig.evaluate();
        cu->evaluate();
        int next_pc = bools_to_int(cu->get_pc_outputs(), pc_bits);
        int expected_next = (jump_target + 1) & ((1 << pc_bits) - 1);
        pass = (next_pc == expected_next);
        ++total_tests;
        
        if (!pass || print_all)
        {
            std::cout << (pass ? "✓ " : "✗ ") << "PC Resume Increment: PC = " << next_pc 
                      << " (expected " << expected_next << ")\n";
        }
        if (!pass) ++failures;
    }
    
    // === TEST 3: Opcode Decoder ===
    std::cout << "\n--- Test 3: Opcode Decoder ---\n";
    {
        int num_opcodes = (1 << num_bits);
        
        for (int opcode = 0; opcode < num_opcodes && opcode < 8; ++opcode)  // Test first 8 opcodes
        {
            set_value(opcode_sigs, 0, opcode, num_bits);
            for (auto& sig : opcode_sigs) sig.evaluate();
            
            cu->evaluate();
            
            // Check that exactly one decoder output is high
            bool* decoder_outputs = cu->get_decoder_outputs();
            int high_count = 0;
            int high_index = -1;
            
            for (int i = 0; i < num_opcodes; ++i)
            {
                if (decoder_outputs[i])
                {
                    high_count++;
                    high_index = i;
                }
            }
            
            bool pass = (high_count == 1 && high_index == opcode);
            ++total_tests;
            
            if (!pass || print_all)
            {
                std::cout << (pass ? "✓ " : "✗ ") << "Opcode " << opcode 
                          << ": decoder[" << high_index << "] high (count=" << high_count << ")\n";
            }
            if (!pass) ++failures;
        }
    }
    
    // === TEST 4: Comparator Flags Storage ===
    std::cout << "\n--- Test 4: Comparator Flags Storage ---\n";
    {
        // Set specific flag pattern (e.g., EQ=1, NEQ=0, LT_U=1, GT_U=0, LT_S=0, GT_S=1)
        flag_sigs[0].go_high();   // EQ
        flag_sigs[1].go_low();    // NEQ
        flag_sigs[2].go_high();   // LT_U
        flag_sigs[3].go_low();    // GT_U
        flag_sigs[4].go_low();    // LT_S
        flag_sigs[5].go_high();   // GT_S
        
        for (auto& sig : flag_sigs) sig.evaluate();
        cu->evaluate();
        
        bool* stored_flags = cu->get_stored_flags();
        bool pass = (stored_flags[0] == true && stored_flags[1] == false &&
                     stored_flags[2] == true && stored_flags[3] == false &&
                     stored_flags[4] == false && stored_flags[5] == true);
        ++total_tests;
        
        if (!pass || print_all)
        {
            std::cout << (pass ? "✓ " : "✗ ") << "Flags Storage: [";
            for (int i = 0; i < 6; ++i)
            {
                std::cout << stored_flags[i];
                if (i < 5) std::cout << ",";
            }
            std::cout << "] (expected [1,0,1,0,0,1])\n";
        }
        if (!pass) ++failures;
    }
    
    // === TEST 5: RAM Page Register ===
    std::cout << "\n--- Test 5: RAM Page Register ---\n";
    {
        // Write a page value
        int page_value = 123 % (1 << pc_bits);
        set_value(page_data_sigs, 0, page_value, pc_bits);
        for (auto& sig : page_data_sigs) sig.evaluate();
        
        page_write_enable_sig.go_high();
        page_write_enable_sig.evaluate();
        
        cu->evaluate();
        int stored_page = bools_to_int(cu->get_ram_page_outputs(), pc_bits);
        bool pass = (stored_page == page_value);
        ++total_tests;
        
        if (!pass || print_all)
        {
            std::cout << (pass ? "✓ " : "✗ ") << "RAM Page Write: " << stored_page 
                      << " (expected " << page_value << ")\n";
        }
        if (!pass) ++failures;
        
        // Disable write and verify value persists
        page_write_enable_sig.go_low();
        page_write_enable_sig.evaluate();
        set_value(page_data_sigs, 0, 0, pc_bits);  // Change input
        for (auto& sig : page_data_sigs) sig.evaluate();
        
        cu->evaluate();
        stored_page = bools_to_int(cu->get_ram_page_outputs(), pc_bits);
        pass = (stored_page == page_value);  // Should still be old value
        ++total_tests;
        
        if (!pass || print_all)
        {
            std::cout << (pass ? "✓ " : "✗ ") << "RAM Page Persistence: " << stored_page 
                      << " (expected " << page_value << ")\n";
        }
        if (!pass) ++failures;
    }
    
    // === Summary ===
    std::cout << "\nControl Unit Test Summary: " << total_tests << " tests, ";
    if (failures == 0)
        std::cout << "✓ ALL PASS\n";
    else
        std::cout << "✗ " << failures << " FAILURES\n";
    
    // Cleanup
    delete[] jump_addr_ptrs;
    delete[] opcode_ptrs;
    delete[] flag_ptrs;
    delete[] page_ptrs;
    delete cu;
}
