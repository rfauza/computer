#pragma once
#include "Device.hpp"
#include "../device_components/Memory_Bit.hpp"
#include <vector>

/**
 * @brief Register that stores num_bits of data using Memory_Bit cells
 * 
 * A register is an array of Memory_Bit cells that share control signals.
 * All bits share the same write_enable and read_enable signals, but each
 * bit has its own data input and output.
 * 
 * Input layout (num_bits + 2 total):
 *   - inputs[0] to inputs[num_bits-1]: data inputs for each bit
 *   - inputs[num_bits]: write_enable (shared)
 *   - inputs[num_bits+1]: read_enable (shared)
 * 
 * Output layout (num_bits total):
 *   - outputs[0] to outputs[num_bits-1]: stored data from each bit
 */
class Register : public Device
{
public:
    /**
     * @brief Constructs a register with num_bits storage capacity
     * 
     * @param num_bits Width of the register (number of bits to store)
     * @param name Optional name identifier for this component
     */
    Register(uint16_t num_bits, const std::string& name = "");
    
    /**
     * @brief Destructor
     */
    ~Register() override;
    
    /**
     * @brief Connects upstream output to a specific input
     */
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    
    /**
     * @brief Evaluates all memory bits and updates outputs
     */
    void evaluate() override;

    /** Returns the raw stored Q value for the given bit, bypassing the read-enable gate. */
    bool get_stored_bit(uint16_t bit) const;

    /** Directly zeroes all stored bits without touching external connections. */
    void zero();

    /** Directly forces a single bit to the given value without touching external connections. */
    void set_bit(uint16_t bit, bool value);

    /** Returns the number of data bits stored in this register. */
    uint16_t get_num_bits() const { return static_cast<uint16_t>(memory_bits.size()); }

    /**
     * @brief Performs update cycle: evaluates all memory bits and signals downstream
     */
    // void update() override;

private:
    std::vector<Memory_Bit*> memory_bits;  /**< Array of Memory_Bit cells */
};

