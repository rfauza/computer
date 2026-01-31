#pragma once
#include "../components/Component.hpp"
#include "../components/AND_Gate.hpp"
#include "../components/Inverter.hpp"
#include "Flip_Flop.hpp"

/**
 * @brief Single bit of memory with write and read control
 * 
 * A memory cell that stores a single bit using a Flip_Flop, with AND gates
 * for write and read control.
 * - Data input (input[0]): The value to write
 * - Write Enable (input[1]): Enables writing to memory
 * - Read Enable (input[2]): Enables reading from memory
 * - Output (output[0]): The stored data when read is enabled
 */
class Memory_Bit : public Component
{
public:
    Memory_Bit();
    ~Memory_Bit() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;

private:
    Inverter data_inverter;     // Inverts data for reset path
    AND_Gate set_and;           // Data AND Write Enable → Set
    AND_Gate reset_and;         // NOT Data AND Write Enable → Reset
    AND_Gate output_and;        // Q AND Read Enable → Output
    Flip_Flop flip_flop;        // SR latch for storage
};

