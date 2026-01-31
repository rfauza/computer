#pragma once
#include "Component.hpp"
#include "Buffer.hpp"
#include "Inverter.hpp"
#include "AND_Gate.hpp"
#include "OR_Gate.hpp"

class XOR_Gate : public Component
{
public:
    XOR_Gate(uint16_t num_bits = 1);
    ~XOR_Gate() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;
    
private:
    Buffer buffer_A;
    Buffer buffer_B;
    AND_Gate and_gate1;
    AND_Gate and_gate2;
    OR_Gate output_or_gate;
    Inverter input_inverter_A;
    Inverter input_inverter_B;
};

