#pragma once
#include "Device.hpp"

/**
 * @brief Right shift device that shifts all input bits right by one position
 * 
 * Performs a right shift (>>) operation on the input bits. Since LSB is at index 0,
 * right shift moves values towards higher indices.
 * 
 * Input layout (num_bits total):
 *   - inputs[0] to inputs[num_bits-1]: the bits to shift
 * 
 * Output layout (num_bits total):
 *   - outputs[0]: always 0 (LSB fills with 0)
 *   - outputs[1] to outputs[num_bits-1]: shifted input bits from lower indices
 * 
 * Example with 4 bits:
 *   - Input:  [1, 0, 1, 1] (indices 0-3, LSB first)
 *   - Output: [0, 1, 0, 1] (1 position right with 0 fill)
 */
class R_Shift : public Device
{
public:
    /**
     * @brief Constructs a right shift device
     * 
     * @param num_bits Width of the value to shift
     */
    R_Shift(uint16_t num_bits, const std::string& name = "");
    
    /**
     * @brief Destructor
     */
    ~R_Shift() override;
    
    /**
     * @brief Performs the right shift operation and updates downstream components
     */
    void update() override;
};
