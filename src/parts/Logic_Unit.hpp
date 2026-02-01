#pragma once
#include "Part.hpp"
#include "../components/AND_Gate.hpp"
#include "../components/OR_Gate.hpp"
#include "../components/XOR_Gate.hpp"
#include "../components/Inverter.hpp"

class Logic_Unit : public Part
{
public:
    Logic_Unit(uint16_t num_bits);
    ~Logic_Unit() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
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

