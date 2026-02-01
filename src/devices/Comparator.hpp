#pragma once
#include "Device.hpp"
#include "../components/NOR_Gate.hpp"

class Comparator : public Device
{
public:
    Comparator(uint16_t num_bits);
    ~Comparator() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;
    
private:
    NOR_Gate zero_flag_nor;
    bool** raw_sum;      // Alias to inputs[0..num_bits-1]
    bool* final_carry;   // Pointer to inputs[num_bits] (final carry from adder)
    bool* a_msb;         // Pointer to A MSB input for overflow calculation
    bool* b_msb;         // Pointer to B MSB input for overflow calculation
    bool* subtract_flag; // Pointer to subtract flag input for overflow calculation
    
    // Output flags: [0] = Z (zero flag), [1] = N (negative), [2] = C (carry), [3] = V (overflow)
};
