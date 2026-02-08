#pragma once
#include "Part.hpp"
#include "../components/OR_Gate.hpp"
#include "../devices/Adder_Subtractor.hpp"
#include "../devices/Multiplier.hpp"

/**
 * @brief Arithmetic unit providing addition, subtraction, and multiplication
 * 
 * Combines arithmetic operations into a single Part.
 * Performs one operation at a time based on enable signals.
 * 
 * Supported operations:
 *   - ADD: A + B
 *   - SUB: A - B
 *   - INC: A + 1
 *   - DEC: A - 1
 *   - MUL: A * B (combinational, single cycle)
 * 
 * Input layout (2*num_bits + 5 total):
 *   - inputs[0] to inputs[num_bits-1]: data_a (first operand)
 *   - inputs[num_bits] to inputs[2*num_bits-1]: data_b (second operand)
 *   - inputs[2*num_bits]: add_enable (enable addition)
 *   - inputs[2*num_bits+1]: sub_enable (enable subtraction)
 *   - inputs[2*num_bits+2]: inc_enable (enable increment)
 *   - inputs[2*num_bits+3]: dec_enable (enable decrement)
 *   - inputs[2*num_bits+4]: mul_enable (enable multiplication)
 * 
 * Output layout (num_bits total):
 *   - outputs[0] to outputs[num_bits-1]: Result of selected operation (lower num_bits of multiply product)
 * 
 * Usage:
 *   - Set exactly ONE enable signal HIGH to select operation
 *   - Multiple enables HIGH will produce undefined behavior
 *   - All enables LOW will produce undefined output
 * 
 * Example (4-bit):
 *   A=0011 (3), B=0010 (2), add_enable=1 → Output=0101 (5)
 *   A=0101 (5), B=0010 (2), sub_enable=1 → Output=0011 (3)
 *   A=0011 (3), B=0010 (2), mul_enable=1 → Output=00000110 (6)
 * 
 * Component of:
 *   - ALU (combined with Logic_Unit and Comparator)
 */
class Arithmetic_Unit : public Part
{
public:
    /**
     * @brief Constructs an arithmetic unit with specified bit width
     * 
     * @param num_bits Width of data operands and output
     */
    Arithmetic_Unit(uint16_t num_bits);
    
    ~Arithmetic_Unit() override;
    
    /**
     * @brief Connects an input signal to the arithmetic unit
     * 
     * @param upstream_output_p Pointer to upstream signal
     * @param input_index Input index (see class documentation for layout)
     * @return true if connection successful
     */
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    
    /**
     * @brief Evaluates the selected arithmetic operation
     * 
     * Checks enable signals and evaluates only the selected operation.
     */
    void evaluate() override;
    
    /**
     * @brief Updates the arithmetic unit and propagates to downstream components
     */
    void update() override;
    
    /**
     * @brief Debug: Print adder_subtractor inputs
     */
    void print_adder_inputs() const;
    /**
     * @brief Debug: Print multiplier internal IO
     */
    void print_multiplier_io() const;

private:
    Adder_Subtractor adder_subtractor;
    Multiplier multiplier;
    OR_Gate* adder_output_enable_or;    // 4-input OR gate for add/sub/inc/dec enables
    OR_Gate* adder_subtract_enable_or;  // 2-input OR gate for sub/dec enables
    
    bool** data_a;            // Alias to inputs[0..num_bits-1]
    bool** data_b;            // Alias to inputs[num_bits..2*num_bits-1]
    bool* add_enable;         // Pointer to inputs[2*num_bits]
    bool* sub_enable;         // Pointer to inputs[2*num_bits+1]
    bool* inc_enable;         // Pointer to inputs[2*num_bits+2]
    bool* dec_enable;         // Pointer to inputs[2*num_bits+3]
    bool* mul_enable;         // Pointer to inputs[2*num_bits+4]
};
