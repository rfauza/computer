#pragma once
#include "Part.hpp"
#include "Arithmetic_Unit.hpp"
#include "Logic_Unit.hpp"
#include "../devices/Comparator.hpp"

/**
 * @brief Arithmetic Logic Unit combining arithmetic and logic operations
 * 
 * The ALU combines an Arithmetic_Unit and Logic_Unit to provide all fundamental
 * operations on data. Performs one operation at a time based on enable signals.
 * 
 * Supported operations (in order):
 *   Arithmetic: ADD, SUB, INC, DEC, MUL
 *   Logic: AND, OR, XOR, NOT, R_SHIFT, L_SHIFT
 * 
 * Input layout (2*num_bits + 11 total):
 *   - inputs[0] to inputs[num_bits-1]: data_a (first operand)
 *   - inputs[num_bits] to inputs[2*num_bits-1]: data_b (second operand)
 *   - inputs[2*num_bits+0]: add_enable
 *   - inputs[2*num_bits+1]: sub_enable
 *   - inputs[2*num_bits+2]: inc_enable
 *   - inputs[2*num_bits+3]: dec_enable
 *   - inputs[2*num_bits+4]: mul_enable
 *   - inputs[2*num_bits+5]: and_enable
 *   - inputs[2*num_bits+6]: or_enable
 *   - inputs[2*num_bits+7]: xor_enable
 *   - inputs[2*num_bits+8]: not_enable
 *   - inputs[2*num_bits+9]: r_shift_enable
 *   - inputs[2*num_bits+10]: l_shift_enable
 * 
 * Output layout (num_bits + 6 total):
 *   - outputs[0] to outputs[num_bits-1]: Result of selected operation
 *   - outputs[num_bits+0]: EQ (A == B)
 *   - outputs[num_bits+1]: NEQ (A != B)
 *   - outputs[num_bits+2]: LT_U (A < B unsigned)
 *   - outputs[num_bits+3]: GT_U (A > B unsigned)
 *   - outputs[num_bits+4]: LT_S (A < B signed)
 *   - outputs[num_bits+5]: GT_S (A > B signed)
 * 
 * Usage:
 *   - Set exactly ONE enable signal HIGH to select operation
 *   - Multiple enables HIGH will produce undefined behavior (both units may output)
 *   - All enables LOW will produce undefined output
 */
class ALU : public Part
{
public:
    /**
     * @brief Constructs an ALU with specified bit width
     * 
     * @param num_bits Width of data operands and output
     */
    ALU(uint16_t num_bits);
    
    ~ALU() override;
    
    /**
     * @brief Connects an input signal to the ALU
     * 
     * @param upstream_output_p Pointer to upstream signal
     * @param input_index Input index (see class documentation for layout)
     * @return true if connection successful
     */
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    
    /**
     * @brief Evaluates the selected operation
     * 
     * Checks enable signals and evaluates the appropriate unit.
     */
    void evaluate() override;
    
    /**
     * @brief Updates the ALU and propagates to downstream components
     */
    void update() override;

    /**
     * @brief Debug helper: print the comparator IO inside the ALU
     */
    void print_comparator_io() const;
private:
    Arithmetic_Unit* arithmetic_unit;
    Logic_Unit* logic_unit;
    Comparator* comparator;
    
    bool* add_enable;
    bool* sub_enable;
    bool* inc_enable;
    bool* dec_enable;
    bool* mul_enable;
    bool* and_enable;
    bool* or_enable;
    bool* xor_enable;
    bool* not_enable;
    bool* r_shift_enable;
    bool* l_shift_enable;
};

