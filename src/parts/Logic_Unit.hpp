#pragma once
#include "Part.hpp"
#include "../components/AND_Gate.hpp"
#include "../components/OR_Gate.hpp"
#include "../components/XOR_Gate.hpp"
#include "../components/Inverter.hpp"

/**
 * @brief Logic unit providing bitwise and shift operations
 * 
 * Combines all fundamental logic and shift operations into a single Part.
 * Performs one operation at a time based on enable signals. All operations
 * are purely combinational (no clock/registers).
 * 
 * Supported operations:
 *   - AND: Bitwise AND of A and B
 *   - OR: Bitwise OR of A and B
 *   - XOR: Bitwise XOR of A and B
 *   - NOT: Bitwise inversion of A (B ignored)
 *   - R_SHIFT: Logical right shift of A (shifts in zeros)
 *   - L_SHIFT: Logical left shift of A (shifts in zeros)
 * 
 * Input layout (2*num_bits + 6 total):
 *   - inputs[0] to inputs[num_bits-1]: data_a (first operand)
 *   - inputs[num_bits] to inputs[2*num_bits-1]: data_b (second operand)
 *   - inputs[2*num_bits]: and_enable (enable AND operation)
 *   - inputs[2*num_bits+1]: or_enable (enable OR operation)
 *   - inputs[2*num_bits+2]: xor_enable (enable XOR operation)
 *   - inputs[2*num_bits+3]: not_enable (enable NOT operation)
 *   - inputs[2*num_bits+4]: r_shift_enable (enable right shift)
 *   - inputs[2*num_bits+5]: l_shift_enable (enable left shift)
 * 
 * Output layout (num_bits total):
 *   - outputs[0] to outputs[num_bits-1]: Result of selected operation
 * 
 * Usage:
 *   - Set exactly ONE enable signal HIGH to select operation
 *   - Multiple enables HIGH will produce undefined behavior
 *   - All enables LOW will produce undefined output
 * 
 * Example (4-bit):
 *   A=1010, B=1100, and_enable=1 → Output=1000
 *   A=1010, B=xxxx, not_enable=1 → Output=0101
 *   A=1010, B=xxxx, l_shift_enable=1 → Output=0100
 * 
 * Component of:
 *   - ALU (combined with Arithmetic_Unit and Comparator)
 */
class Logic_Unit : public Part
{
public:
    /**
     * @brief Constructs a logic unit with specified bit width
     * 
     * @param num_bits Width of data operands and output
     */
    Logic_Unit(uint16_t num_bits, const std::string& name = "");
    
    ~Logic_Unit() override;
    
    /**
     * @brief Connects an input signal to the logic unit
     * 
     * @param upstream_output_p Pointer to upstream signal
     * @param input_index Input index (see class documentation for layout)
     * @return true if connection successful
     */
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    
    /**
     * @brief Evaluates the selected logic operation
     * 
     * Checks enable signals and evaluates only the selected operation.
     * Shift operations are performed directly; gate operations use components.
     */
    void evaluate() override;
    
    /**
     * @brief Updates the logic unit and propagates to downstream components
     */
    void update() override;
    
private:
    AND_Gate* and_gates;
    OR_Gate* or_gates;
    XOR_Gate* xor_gates;
    Inverter* not_gates;
    
    bool** data_a;            // Alias to inputs[0..num_bits-1]
    bool** data_b;            // Alias to inputs[num_bits..2*num_bits-1]
    bool* and_enable;         // Pointer to inputs[2*num_bits]
    bool* or_enable;          // Pointer to inputs[2*num_bits+1]
    bool* xor_enable;         // Pointer to inputs[2*num_bits+2]
    bool* not_enable;         // Pointer to inputs[2*num_bits+3]
    bool* r_shift_enable;     // Pointer to inputs[2*num_bits+4]
    bool* l_shift_enable;     // Pointer to inputs[2*num_bits+5]
};

