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
    CPU(uint16_t num_bits, const std::string& opcode_string, const std::string& name = "");
    
    ~CPU() override;
    
    /**
     * @brief Evaluates CPU internal components
     */
    void evaluate() override;
    
    /**
     * @brief Updates CPU and propagates signals
     */
    void update() override;
    
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
     * @param data_c_outputs Pointer to C register/immediate data
     * @param data_a_outputs Pointer to A register data
     * @param data_b_outputs Pointer to B register data
     * @return true if successful
     */
    bool connect_data_inputs(
        const bool* const* data_c_outputs,
        const bool* const* data_a_outputs,
        const bool* const* data_b_outputs
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
     * Creates OR gates to combine multiple decoder outputs to each ALU enable
     */
    void wire_decoder_to_alu();
    
    Control_Unit* control_unit;  /**< Control unit with PC, decoder, flags */
    ALU* alu;                    /**< Arithmetic Logic Unit */
    
    std::map<std::string, uint16_t> operation_to_opcode;  /**< Maps operation names to opcodes */
    std::map<uint16_t, std::string> opcode_to_operation;  /**< Maps opcodes to operation names */
    
    // OR gates to combine decoder outputs to ALU enables
    // (multiple opcodes can map to same ALU operation)
    OR_Gate* add_enable_or;      /**< Combines opcodes that trigger ADD */
    OR_Gate* sub_enable_or;      /**< Combines opcodes that trigger SUB */
    OR_Gate* inc_enable_or;      /**< Combines opcodes that trigger INC */
    OR_Gate* dec_enable_or;      /**< Combines opcodes that trigger DEC */
    OR_Gate* mul_enable_or;      /**< Combines opcodes that trigger MUL */
    OR_Gate* and_enable_or;      /**< Combines opcodes that trigger AND */
    OR_Gate* or_enable_or;       /**< Combines opcodes that trigger OR */
    OR_Gate* xor_enable_or;      /**< Combines opcodes that trigger XOR */
    OR_Gate* not_enable_or;      /**< Combines opcodes that trigger NOT */
    OR_Gate* rsh_enable_or;      /**< Combines opcodes that trigger RSH */
    OR_Gate* lsh_enable_or;      /**< Combines opcodes that trigger LSH */
    
    Signal_Generator* low_signal; /**< Always-low signal for unused OR gate inputs */
    
    uint16_t num_decoder_outputs;  /**< Number of decoder outputs (2^opcode_bits) */
    uint16_t opcode_bits;          /**< Number of opcode bits */
};

