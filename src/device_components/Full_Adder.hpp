#pragma once
#include "../components/Component.hpp"
#include "../components/OR_Gate.hpp"
#include "Half_Adder.hpp"

class Full_Adder : public Component
{
public:
    Full_Adder();
    ~Full_Adder() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;
    
private:
    Half_Adder half_adder_1;
    Half_Adder half_adder_2;
    OR_Gate or_gate_1{2};
};

