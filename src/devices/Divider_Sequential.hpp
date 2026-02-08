#pragma once
#include "Device.hpp"
#include "Register.hpp"
#include "Adder_Subtractor.hpp"
#include "L_Shift.hpp"
#include "../device_components/Flip_Flop.hpp"
#include "../components/AND_Gate.hpp"
#include "../components/Signal_Generator.hpp"

/**
 * @brief Sequential restoring division using shift-and-subtract algorithm
 * 
 * Implements binary long division using iterative shift-and-subtract with
 * comparison. Similar to sequential multiplication but uses subtraction
 * instead of addition.
 * 
 * Algorithm (for each bit, MSB to LSB):
 *   1. Shift remainder left, bring in next bit of dividend
 *   2. Try: temp = remainder - divisor
 *   3. If temp >= 0 (no borrow):
 *      - remainder = temp
 *      - Set current quotient bit to 1
 *   4. Else (temp < 0, borrow occurred):
 *      - Keep old remainder (restore)
 *      - Set current quotient bit to 0
 *   5. Shift quotient left
 *   6. Repeat for num_bits cycles
 * 
 * Architecture:
 *   - 4 registers: quotient (num_bits), remainder (num_bits),
 *     divisor (num_bits), busy_flag (1 bit)
 *   - 1 adder_subtractor (num_bits wide)
 *   - Left shift logic
 *   - Hardware complexity: O(n) gates (~150 gates for 16-bit)
 *   - Execution time: num_bits clock cycles
 * 
 * Input layout (2*num_bits + 1 total):
 *   - inputs[0] to inputs[num_bits-1]: dividend (numerator)
 *   - inputs[num_bits] to inputs[2*num_bits-1]: divisor (denominator)
 *   - inputs[2*num_bits]: start signal (trigger division)
 * 
 * Output layout (2*num_bits + 1 total):
 *   - outputs[0] to outputs[num_bits-1]: quotient (dividend / divisor)
 *   - outputs[num_bits] to outputs[2*num_bits-1]: remainder (dividend % divisor)
 *   - outputs[2*num_bits]: busy flag (HIGH while computing)
 * 
 * Usage:
 *   1. Connect dividend and divisor inputs
 *   2. Call start() to load operands and begin division
 *   3. Call evaluate() each cycle until is_busy() returns false
 *   4. Read quotient from outputs[0..num_bits-1]
 *   5. Read remainder from outputs[num_bits..2*num_bits-1]
 * 
 * CPU Integration:
 *   - Wire outputs[2*num_bits] (busy) to CPU busy_flag register
 *   - PC.write_enable = !busy_flag (freezes PC during division)
 * 
 * Division by zero:
 *   - No hardware check - produces undefined result
 *   - Software must check divisor != 0 before calling
 * 
 * Trade-offs:
 *   - 4-bit: ~30 gates, 4 cycles
 *   - 16-bit: ~150 gates, 16 cycles
 *   - Much cheaper than combinational divider (~2000+ gates)
 *   - Still expensive compared to software division (0 gates, 200+ cycles)
 */
class Divider_Sequential : public Device
{
public:
    /**
     * @brief Constructs a sequential divider
     * 
     * @param num_bits Width of dividend, divisor, quotient, and remainder
     */
    Divider_Sequential(uint16_t num_bits);
    
    ~Divider_Sequential() override;
    
    /**
     * @brief Connects an input signal to the divider
     * 
     * @param upstream_output_p Pointer to upstream signal
     * @param input_index Input index (see class documentation for layout)
     * @return true if connection successful
     */
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    
    /**
     * @brief Performs one cycle of the division algorithm
     * 
     * If busy:
     *   - Shifts remainder left, brings in next dividend bit
     *   - Subtracts divisor from remainder
     *   - If no borrow (carry flag): updates remainder, sets quotient bit to 1
     *   - If borrow: restores remainder, sets quotient bit to 0
     *   - Shifts quotient left
     *   - Increments cycle counter
     *   - Clears busy flag when num_bits cycles completed
     */
    void evaluate() override;
    
    /**
     * @brief Updates the divider and propagates to downstream components
     */
    void update() override;
    
    // Sequential control
    
    /**
     * @brief Begins division operation
     * 
     * Loads dividend into remainder, loads divisor, clears quotient,
     * sets busy flag HIGH, and resets cycle counter.
     * Must be called before calling evaluate().
     */
    void start();
    
    /**
     * @brief Checks if division is in progress
     * 
     * @return true if busy (still computing), false if result ready
     */
    bool is_busy() const;
    
private:
    // Registers
    Register* quotient;          // Holds quotient result (num_bits)
    Register* remainder;         // Holds remainder during computation (num_bits)
    Register* divisor;           // Holds divisor value (num_bits)
    Flip_Flop* busy_flag;        // Single bit busy status
    
    // Combinational logic
    Adder_Subtractor* subtractor;  // Subtracts divisor from remainder
    L_Shift* shift_left_rem;       // Shifts remainder left
    L_Shift* shift_left_quot;      // Shifts quotient left
    
    // Control signals
    Signal_Generator* write_enable;   // For register write control
    Signal_Generator* read_enable;    // For register read control
    Signal_Generator* zero_signal;    // Constant zero
    Signal_Generator* one_signal;     // Constant one
    
    // Output gating
    AND_Gate* output_AND_gates;      // AND gates for gating outputs with output_enable
    bool* output_enable;              // Pointer to inputs[2*num_bits+1]
    
    // State
    uint16_t cycle_count;
    bool* dividend_bits;         // Copy of dividend for bit-by-bit access
};
