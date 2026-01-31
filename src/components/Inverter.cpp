#include "Inverter.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

Inverter::Inverter(uint16_t num_inputs_param)
{
    num_inputs = num_inputs_param;
    std::ostringstream oss;
    oss << "Inverter 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    num_outputs = num_inputs;
    initialize_IO_arrays();
}

Inverter::~Inverter()
{
}

void Inverter::evaluate()
{
    // Invert each bit: Output[i] = NOT Input[i]
    for (uint16_t i = 0; i < num_inputs; ++i) {
        if (inputs[i] == nullptr) {
            std::cerr << "Error: " << component_name << " - input[" << i << "] not connected" << std::endl;
            return;
        }
        outputs[i] = !(*inputs[i]);
    }
}

