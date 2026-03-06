#pragma once
#include "Part.hpp"
#include "Control_Unit.hpp"
#include "ALU.hpp"
#include "../components/Signal_Generator.hpp"
#include "../components/OR_Gate.hpp"
#include <map>
#include <string>
#include <vector>

/**
 * @brief Central Processing Unit integrating Control Unit and ALU
 * 
 * The CPU combines a Control Unit (CU) and Arithmetic Logic Unit (ALU) to execute
 * instructions. It parses an opcode specification string to configure the mapping
 * between opcodes and ALU operations.
 * 
 * Opcode string format (one per line):
 *   "0000 HALT
 *    0001 ADD
 *    0010 SUB
 *    ..."
 * 
 * Supported operation names:
 *   - Arithmetic: ADD, SUB, INC, DEC, MUL
 *   - Logic: AND, OR, XOR, NOT, RSH, LSH
 *   - Control: HALT, NOP, JMP, CMP
 *   - Memory: LOAD, STORE, PAGE
 */
class CPU : public Part
{
public:
    /**
     * @brief Constructs a CPU with specified bit width and opcode mapping
     * 
     * @param num_bits Base bit width for data operations
     * @param opcode_string Newline-separated opcode definitions ("0000 ADD\n0001 SUB\n...")
     * @param name Optional component name
     */
    CPU(uint16_t num_bits, const std::string& opcode_string, const std::string& name = "", uint16_t pc_bits = 4);
    
    ~CPU() override;
    
    /**
     * @brief Evaluates CPU internal components
     */
    void evaluate() override;
    
    // === External Connection Methods ===
    
    /**
     * @brief Connect Program Memory to CPU
     * 
     * Connects PC output to PM address input and PM opcode output to CU decoder
     * 
     * @param pm_opcode_outputs Pointer array to PM opcode outputs
     * @param pm_address_inputs Pointer array to PM address inputs
     * @return true if successful
     */
    bool connect_program_memory(const bool* const* pm_opcode_outputs, bool** pm_address_inputs);
    
    /**
     * @brief Connect data inputs from Program Memory or RAM
     * 
     * @param data_a_outputs Pointer to A register data
     * @param data_b_outputs Pointer to B register data
     * @param data_c_outputs Pointer to C register/immediate data
     * @return true if successful
     */
    bool connect_data_inputs(
        const bool* const* data_a_outputs,
        const bool* const* data_b_outputs,
        const bool* const* data_c_outputs
    );
    
    /**
     * @brief Get pointer to ALU result outputs
     * 
     * @return Pointer to result output array (num_bits)
     */
    bool* get_result_outputs() const;
    
    /**
     * @brief Get pointer to PC outputs for external monitoring
     * 
     * @return Pointer to PC output array (2*num_bits)
     */
    bool* get_pc_outputs() const;
    
    /**
     * @brief Get run/halt flag state from control unit
     * 
     * @return true if CPU is running, false if halted
     */
    bool get_run_halt_flag() const;

    /**
     * @brief Set the run/halt flag in the control unit.
     * @param state true = running, false = halted
     */
    void set_run_halt_flag(bool state);

    /**
     * @brief Reset the program counter to address 0.
     */
    void reset_pc();
    
    /**
     * @brief Get opcode value for a given operation name
     * 
     * @param operation_name Name of operation (e.g., "ADD", "SUB")
     * @return Opcode value, or -1 if not found
     */
    int get_opcode_for_operation(const std::string& operation_name) const;
    
    /**
     * @brief Trigger a clock cycle for sequential logic
     */
    void clock_tick();
    
    /**
     * @brief Get pointer to decoder outputs for instruction decoding
     * 
     * @return Pointer to decoder output array (2^opcode_bits)
     */
    bool* get_decoder_outputs() const;
    
    /**
     * @brief Get pointer to stored comparator flags
     * 
     * @return Pointer to stored comparator flags array
     */
    bool* get_cmp_flags() const;

    /**
     * @brief Wire the flag write-enable to an external signal
     *
     * @param signal_ptr Pointer to signal (e.g., decoder output for CMP)
     * @return true if successful
     */
    bool wire_flag_write_enable(const bool* signal_ptr);
    
    /**
     * @brief Get number of opcode bits
     */
    uint16_t get_opcode_bits() const { return opcode_bits; }
    
    /**
     * @brief Wire a specific opcode to the halt signal
     * 
     * Connects decoder output for the given opcode to Control Unit halt signal.
     * This allows the HALT instruction to stop execution.
     * 
     * @param halt_opcode_value The opcode value (e.g., 0 for "000" opcode)
     * @return true if successful
     */
    bool wire_halt_opcode(uint16_t halt_opcode_value);
    
    /**
     * @brief Connect conditional jump instructions with their comparator flags
     * 
     * Sets up logic to AND jump instruction decoder outputs with appropriate comparator flags,
     * then ORs all jump conditions together to drive the jump_enable signal.
     * This enables conditional jumping based on comparison results.
     * 
     * @param jump_operation_flag_pairs Vector of (operation_name, flag_index) pairs
     *   e.g., {("JEQ", 0), ("JGT", 3)} means JEQ ANDed with EQ flag, JGT ANDed with GT_U flag
     * @return true if successful
     */
    bool connect_jump_conditions(const std::vector<std::pair<std::string, uint16_t>>& jump_operation_flag_pairs);
    
    /**
     * @brief Connect jump address from external source
     * 
     * @param jump_address_outputs Pointer array to jump address bits
     * @param num_address_bits Number of address bits (should match PC width)
     * @return true if successful
     */
    bool connect_jump_address(const bool* const* jump_address_outputs, uint16_t num_address_bits);
    
private:
    /**
     * @brief Parse opcode specification string
     * 
     * Parses string format "0000 HALT\n0001 ADD\n..." into opcode map
     * 
     * @param opcode_string Newline-separated opcode definitions
     */
    void parse_opcodes(const std::string& opcode_string);
    
    /**
     * @brief Wire decoder outputs to ALU enables based on opcode mapping
     * 
     * Directly connects decoder outputs to ALU enable inputs.
     */
    void wire_decoder_to_alu();
    
    Control_Unit* control_unit;  /**< Control unit with PC, decoder, flags */
    ALU* alu;                    /**< Arithmetic Logic Unit */
    
    std::map<std::string, uint16_t> operation_to_opcode;  /**< Maps operation names to opcodes */
    std::map<uint16_t, std::string> opcode_to_operation;  /**< Maps opcodes to operation names */
    
    Signal_Generator* low_signal; /**< Always-low signal for unused OR gate inputs */
    
    uint16_t num_decoder_outputs;  /**< Number of decoder outputs (2^opcode_bits) */
    uint16_t opcode_bits;          /**< Number of opcode bits */
};

