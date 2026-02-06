#pragma once
#include "Device.hpp"
#include "../components/Inverter.hpp"
#include "../components/AND_Gate.hpp"

/**
 * @brief One-hot decoder (n inputs → 2^n outputs)
 * 
 * Builds a standard decoder by inverting each input once and then ANDing
 * the correct combination of true/inverted inputs for each output line.
 * 
 * Input layout (num_inputs total):
 *   - inputs[0] to inputs[num_inputs-1]: selector bits (LSB at index 0)
 * 
 * Output layout (2^num_inputs total):
 *   - outputs[i] is HIGH only when inputs match binary value i
 * 
 * Example (num_inputs = 2):
 *   inputs: 00 → outputs[0]=1
 *   inputs: 01 → outputs[1]=1
 *   inputs: 10 → outputs[2]=1
 *   inputs: 11 → outputs[3]=1
 */
class Decoder : public Device
{
public:
    /**
     * @brief Constructs a decoder with num_inputs selector bits
     * 
     * @param num_bits Number of selector inputs (outputs = 2^num_bits)
     */
    Decoder(uint16_t num_bits);
    
    ~Decoder() override;
    
    /**
     * @brief Connects an input signal to the decoder
     * 
     * @param upstream_output_p Pointer to upstream signal
     * @param input_index Input index (0..num_inputs-1)
     * @return true if connection successful
     */
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    
    /**
     * @brief Evaluates the decoder (propagates through inverters and AND gates)
     */
    void evaluate() override;
    
    /**
     * @brief Updates the decoder and propagates to downstream components
     */
    void update() override;
    
private:
    Inverter* input_inverters; // one per input
    AND_Gate** output_ands; // array of pointers to AND_Gate objects (one per output)
};
