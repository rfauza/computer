#include "Inverter.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

Inverter::Inverter(uint16_t num_bits_param)
{
    num_bits = num_bits_param;
    std::ostringstream oss;
    oss << "Inverter 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    num_inputs = num_bits;
    num_outputs = num_bits;
    initialize_IO_arrays();
}

Inverter::~Inverter()
{
}

void Inverter::evaluate()
{
    // Invert each bit: Output[i] = NOT Input[i]
    for (uint16_t i = 0; i < num_bits; ++i) {
        if (inputs[i] == nullptr) {
            std::cerr << "Error: " << component_name << " - input[" << i << "] not connected" << std::endl;
            return;
        }
        outputs[i] = !(*inputs[i]);
    }
}

