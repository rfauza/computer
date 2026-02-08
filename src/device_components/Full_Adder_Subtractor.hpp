#pragma once
#include "../components/Component.hpp"
#include "../components/XOR_Gate.hpp"
#include "Full_Adder.hpp"

// does not work on its own since only the first one needs to connect K(subtract) input to the carry-in
// subsequent FAS's use carry normally. does work in multi-bit
class Full_Adder_Subtractor : public Component
{
public:
    Full_Adder_Subtractor();
    ~Full_Adder_Subtractor() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;
    
private:
    Full_Adder full_adder;
    XOR_Gate xor_gate_1;
};

