#pragma once
#include "Device.hpp"
#include "../device_components/Full_Adder.hpp"
#include "../components/Signal_Generator.hpp"

class Adder : public Device
{
public:
    Adder(uint16_t num_bits, const std::string& name = "");
    ~Adder() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;
    
private:
    Full_Adder** adders;  // Array of Full_Adder pointers
    Signal_Generator* carry_in_signal;
};
