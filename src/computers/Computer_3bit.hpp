#pragma once
#include "../parts/Part.hpp"
#include "../parts/CPU.hpp"
#include "../parts/Program_Memory.hpp"
#include "../parts/Main_Memory.hpp"
#include "../components/Signal_Generator.hpp"
#include "../components/OR_Gate.hpp"
#include "../components/AND_Gate.hpp"
#include "../components/Inverter.hpp"
#include <string>
#include <vector>

/**
 * @brief 3-bit Computer with CPU, Program Memory, and RAM
 * 
 * Implements the 3-bit ISA v2:
 *   000 HALT   - Stop execution
 *   001 MOVL   - Move literal: litA -> [C]
 *   010 ADD    - Add: [A] + [B] -> [C]
 *   011 SUB    - Subtract: [A] - [B] -> [C]
 *   100 CMP    - Compare: sets flags based on [A] vs [B]
 *   101 JEQ    - Jump if equal: if flag then goto ABC
 *   110 JGT    - Jump if greater: if flag then goto ABC
 *   111 NOP    - No operation
 * 
 * Architecture:
 *   - 3-bit data (values 0-7)
 *   - 8 RAM addresses (3-bit addressing)
 *   - 512 Program Memory addresses (9-bit PC)
 *   - Program format per line: "opcode C A B" (binary or decimal)
 */
class Computer_3bit : public Part
{
public:
    /**
     * @brief Constructs a 3-bit computer
     * 
     * @param name Optional component name
     */
    Computer_3bit(const std::string& name = "");
    
    ~Computer_3bit() override;
    
    /**
     * @brief Load a program from file into Program Memory
     * 
     * File format (one instruction per line):
     *   opcode C A B
     * 
     * Example:
     *   001 101 011 000  # MOVL 3 -> [5]
     *   010 111 101 110  # ADD [5] + [6] -> [7]
     *   000 000 000 000  # HALT
     * 
     * @param filename Path to program file
     * @return true if loaded successfully
     */
    bool load_program(const std::string& filename);
    
    /**
     * @brief Execute the loaded program interactively
     * 
     * Prints PC, current instruction, and all RAM contents.
     * Waits for user input (Enter) before each clock tick.
     * Stops when HALT instruction is executed.
     */
    void run_interactive();
    
    /**
     * @brief Execute one clock cycle
     * 
     * @return true if should continue (not halted), false if halted
     */
    bool clock_tick();
    
    /**
     * @brief Print current state (PC, instruction, RAM)
     */
    void print_state() const;
    
    /**
     * @brief Reset the computer to initial state
     */
    void reset();
    
    void evaluate() override;
    void update() override;

private:
    static constexpr uint16_t NUM_BITS = 3;
    static constexpr uint16_t NUM_RAM_ADDRESSES = 8;     // 2^3
    static constexpr uint16_t PC_BITS = 9;               // Use 9-bit PC for 512 PM addresses
    static constexpr uint16_t NUM_PM_ADDRESSES = 512;    // 2^9
    
    CPU* cpu;
    Program_Memory* program_memory;
    Main_Memory* ram;
    
    // Opcode mappings
    static const std::string ISA_V2_OPCODES;
    
    // Control signals
    Signal_Generator* pm_write_enable;
    Signal_Generator* pm_read_enable;
    Signal_Generator* ram_write_enable;
    Signal_Generator* ram_read_enable;
    OR_Gate* ram_write_or;  // Combines MOVL and ADD opcodes for RAM write enable
    
    // Mux for RAM data input: selects PM A field (MOVL) or CPU result (ADD/SUB)
    Inverter* ram_data_mux_not;
    AND_Gate** ram_data_mux_and_literal;
    AND_Gate** ram_data_mux_and_result;
    OR_Gate** ram_data_mux_or;
    
    // Signal generators for program loading (kept alive to avoid use-after-free)
    // Use pointers to vectors to avoid overflow (vectors are large)
    std::vector<Signal_Generator>* pm_load_addr_sigs;   // 9-bit address for loading
    std::vector<Signal_Generator>* pm_load_data_sigs;   // 12-bit data for loading (opcode, C, A, B)
    
    // Signal generators for RAM address selection during execution
    // Extract A field (bits 6-8) from PM instruction and route to RAM address decoder
    // Mutable: used for reading RAM values in const print_state()
    mutable std::vector<Signal_Generator>* ram_addr_sigs;       // 3-bit RAM address driver
    
    // Pointer arrays for CPU data inputs (must persist, not deleted)
    const bool** data_a_ptrs;  // Pointers to RAM port A outputs
    const bool** data_b_ptrs;  // Pointers to RAM port B outputs
    const bool** data_c_ptrs;  // Pointers to PM A field for MOVL literals
    
    // Helper to convert 3-bit value to binary string
    std::string to_binary(uint16_t value, uint16_t bits) const;
    
    // Helper to convert binary string to int
    uint16_t from_binary(const std::string& binary) const;
    
    // Helper to get opcode name
    std::string get_opcode_name(uint16_t opcode) const;
};
