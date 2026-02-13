#pragma once
#include "Device.hpp"

/**
 * @brief Left shift device that shifts all input bits left by one position
 * 
 * Performs a left shift (<<) operation on the input bits. Since LSB is at index 0,
 * left shift moves values towards lower indices.
 * 
 * Input layout (num_bits total):
 *   - inputs[0] to inputs[num_bits-1]: the bits to shift
 * 
 * Output layout (num_bits total):
 *   - outputs[0] to outputs[num_bits-2]: shifted input bits from higher indices
 *   - outputs[num_bits-1]: always 0 (MSB fills with 0)
 * 
 * Example with 4 bits:
 *   - Input:  [1, 0, 1, 1] (indices 0-3, LSB first)
 *   - Output: [0, 1, 0, 1] (1 position left with 0 fill)
 */
class L_Shift : public Device
{
public:
    /**
     * @brief Constructs a left shift device
     * 
     * @param num_bits Width of the value to shift
     */
    L_Shift(uint16_t num_bits, const std::string& name = "");
    
    /**
     * @brief Destructor
     */
    ~L_Shift() override;
    
    /**
     * @brief Performs the left shift operation and updates downstream components
     */
    void update() override;
};
