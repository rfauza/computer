#pragma once
#include "Device.hpp"
#include "../components/AND_Gate.hpp"
#include "../components/NOR_Gate.hpp"

class Adder_Subtractor : public Device
{
public:
    Adder_Subtractor(uint16_t num_bits, const std::string& name = "");
    ~Adder_Subtractor() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;
    const bool* get_internal_output() const { return internal_output; }
    
private:
    Full_Adder_Subtractor** adder_subtractors;
    AND_Gate** output_AND_gates;
    NOR_Gate* zero_flag_nor;
    bool* internal_output; // Raw sum [0..num_bits-1] + carry_out [num_bits]
    
    bool** data_input; // Alias to Component inputs[0..num_bits-1]
    bool* data_output; // Alias to Component outputs[0..num_bits-1]
    bool* subtract_enable; // Pointer to inputs[2*num_bits]
    bool* output_enable; // Pointer to inputs[2*num_bits+1]
    bool** data_a_input; // Alias for A inputs (for flag computation)
    bool** data_b_input; // Alias for B inputs (for flag computation)
};
