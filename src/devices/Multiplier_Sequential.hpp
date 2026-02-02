#pragma once
#include "Device.hpp"
#include "Register.hpp"
#include "Adder.hpp"
#include "L_Shift.hpp"
#include "R_Shift.hpp"
#include "../device_components/Flip_Flop.hpp"
#include "../components/Signal_Generator.hpp"

/**
 * @brief Sequential shift-and-add multiplier with minimal hardware
 * 
 * Implements binary multiplication using the classic iterative shift-and-add
 * algorithm. Trades execution time for hardware efficiency compared to the
 * combinational array multiplier.
 * 
 * Algorithm (for each bit of B, LSB to MSB):
 *   1. If current bit of B is 1: accumulator += multiplicand
 *   2. Shift multiplicand left by 1
 *   3. Shift multiplier (B) right by 1
 *   4. Repeat for num_bits cycles
 * 
 * Architecture:
 *   - 4 registers: accumulator (2×num_bits), multiplicand (2×num_bits), 
 *     multiplier (num_bits), busy_flag (1 bit)
 *   - 1 adder (2×num_bits wide)
 *   - Left/right shift logic
 *   - Hardware complexity: O(n) gates (15× smaller than array multiplier)
 *   - Execution time: num_bits clock cycles
 * 
 * Input layout (2*num_bits + 1 total):
 *   - inputs[0] to inputs[num_bits-1]: A (multiplicand)
 *   - inputs[num_bits] to inputs[2*num_bits-1]: B (multiplier)
 *   - inputs[2*num_bits]: start signal (trigger multiplication)
 * 
 * Output layout (2*num_bits + 1 total):
 *   - outputs[0] to outputs[2*num_bits-1]: Product (A × B)
 *   - outputs[2*num_bits]: busy flag (HIGH while computing)
 * 
 * Usage:
 *   1. Connect inputs A and B
 *   2. Call start() to load operands and begin multiplication
 *   3. Call evaluate() each cycle until is_busy() returns false
 *   4. Read result from outputs[0..2*num_bits-1]
 * 
 * CPU Integration:
 *   - Wire outputs[2*num_bits] (busy) to CPU busy_flag register
 *   - PC.write_enable = !busy_flag (freezes PC during multiplication)
 * 
 * Trade-offs:
 *   - 4-bit: ~20 gates, 4 cycles
 *   - 16-bit: ~100 gates, 16 cycles (vs ~1500 gates, 1 cycle for array multiplier)
 *   - Use for area-constrained designs or when multiplication is infrequent
 */
class Multiplier_Sequential : public Device
{
public:
    /**
     * @brief Constructs a sequential multiplier
     * 
     * @param num_bits Width of input operands (output is 2*num_bits wide)
     */
    Multiplier_Sequential(uint16_t num_bits);
    
    ~Multiplier_Sequential() override;
    
    /**
     * @brief Connects an input signal to the multiplier
     * 
     * @param upstream_output_p Pointer to upstream signal
     * @param input_index Input index (see class documentation for layout)
     * @return true if connection successful
     */
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    
    /**
     * @brief Performs one cycle of the multiplication algorithm
     * 
     * If busy:
     *   - Conditionally adds multiplicand to accumulator based on LSB of multiplier
     *   - Shifts multiplicand left
     *   - Shifts multiplier right
     *   - Increments cycle counter
     *   - Clears busy flag when num_bits cycles completed
     */
    void evaluate() override;
    
    /**
     * @brief Updates the multiplier and propagates to downstream components
     */
    void update() override;
    
    // Sequential control
    
    /**
     * @brief Begins multiplication operation
     * 
     * Loads A and B into internal registers, clears accumulator,
     * sets busy flag HIGH, and resets cycle counter.
     * Must be called before calling evaluate().
     */
    void start();
    
    /**
     * @brief Checks if multiplication is in progress
     * 
     * @return true if busy (still computing), false if result ready
     */
    bool is_busy() const;
    
private:
    // Registers
    Register* accumulator;       // Holds partial result (2*num_bits)
    Register* multiplicand;      // Holds A (shifted left each cycle)
    Register* multiplier_reg;    // Holds B (shifted right each cycle)
    Flip_Flop* busy_flag;        // Single bit busy status
    
    // Combinational logic
    Adder* adder;                // Adds multiplicand to accumulator
    L_Shift* shift_left;         // Shifts A left
    R_Shift* shift_right;        // Shifts B right
    
    // Control signals
    Signal_Generator* write_enable;   // For register write control
    Signal_Generator* read_enable;    // For register read control
    Signal_Generator* zero_signal;    // Constant zero
    Signal_Generator* one_signal;     // Constant one
    
    uint16_t cycle_count;
};
