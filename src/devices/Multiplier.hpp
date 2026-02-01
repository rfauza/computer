#pragma once
#include "Device.hpp"
#include "Adder.hpp"
#include "../components/AND_Gate.hpp"
#include "../components/Signal_Generator.hpp"

/**
 * @brief Combinational array multiplier using shift-and-add architecture
 * 
 * Implements binary multiplication using a 2D array of AND gates to compute
 * partial products, followed by cascaded adders to sum the shifted products.
 * This is a purely combinational circuit with no clock or registers.
 * 
 * Architecture:
 *   - num_bits × num_bits AND gate array computes all partial products (A[i] AND B[j])
 *   - num_bits-1 cascaded adders of increasing width sum shifted partial products
 *   - Hardware complexity: O(n²) gates
 *   - Propagation delay: O(n) gate delays (combinational, single cycle)
 * 
 * Input layout (2*num_bits total):
 *   - inputs[0] to inputs[num_bits-1]: A (multiplicand)
 *   - inputs[num_bits] to inputs[2*num_bits-1]: B (multiplier)
 * 
 * Output layout (2*num_bits total):
 *   - outputs[0] to outputs[2*num_bits-1]: Product (A × B)
 * 
 * Example (4-bit): 5 × 3 = 15
 *   A=0101, B=0011 → Product=00001111
 * 
 * Trade-offs:
 *   - 4-bit: ~22 full adders, instant result
 *   - 16-bit: ~368 full adders, instant result
 *   - Use for speed-critical applications; see Multiplier_Sequential for area-efficient version
 */
class Multiplier : public Device
{
public:
    /**
     * @brief Constructs a combinational array multiplier
     * 
     * @param num_bits Width of input operands (output is 2*num_bits wide)
     */
    Multiplier(uint16_t num_bits);
    
    ~Multiplier() override;
    
    /**
     * @brief Connects an input signal to the multiplier
     * 
     * Routes A inputs to all AND gate columns, B inputs to all AND gate rows
     * 
     * @param upstream_output_p Pointer to upstream signal
     * @param input_index Input index (0..num_bits-1 for A, num_bits..2*num_bits-1 for B)
     * @return true if connection successful
     */
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    
    /**
     * @brief Evaluates the multiplication (propagates through AND gates and adders)
     * 
     * Computes all partial products then cascades through adder tree.
     * Pure combinational operation - no state changes.
     */
    void evaluate() override;
    
    /**
     * @brief Updates the multiplier and propagates to downstream components
     */
    void update() override;
    
private:
    AND_Gate** and_array;        // [num_bits][num_bits] AND gates for partial products
    Adder** adder_array;         // [num_bits-1] adders of increasing width
    Signal_Generator** zeros;    // Constant zero signals for shifting
};

