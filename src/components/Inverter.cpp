#include "Inverter.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

Inverter::Inverter(uint16_t num_inputs_param, const std::string& name)
    : Component(name)
{
    num_inputs = num_inputs_param;
    // create component name string (include hex address and optional provided name)
    std::ostringstream oss;
    oss << "Inverter 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    std::cerr << "DEBUG: Inverter ctor name_param='" << name << "' actual='" << component_name << "'\n";
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

