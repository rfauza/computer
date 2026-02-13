#pragma once
#include "Component.hpp"
#include "Buffer.hpp"
#include "Inverter.hpp"
#include "AND_Gate.hpp"
#include "OR_Gate.hpp"
#include <vector>

class XOR_Gate : public Component
{
public:
    XOR_Gate(uint16_t num_inputs = 2, const std::string& name = "");
    ~XOR_Gate() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;
    
private:
    std::vector<Buffer*> input_buffers;      // one buffer per input
    std::vector<Inverter*> input_inverters; // one inverter per input
    std::vector<AND_Gate*> and_gates;       // one 2-input AND per input
    OR_Gate* output_or_gate;                // multi-input OR
};

