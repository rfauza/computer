#include "OR_Gate.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

OR_Gate::OR_Gate(uint16_t num_inputs_param, const std::string& name)
    : Component(name)
{
    num_inputs = num_inputs_param;
    // create component name string (include hex address and optional provided name)
    std::ostringstream oss;
    oss << "OR_Gate 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    num_outputs = 1;
    initialize_IO_arrays();
}

OR_Gate::~OR_Gate()
{
}

void OR_Gate::evaluate()
{
    // OR all input bits together
    outputs[0] = false;
    for (uint16_t i = 0; i < num_inputs; ++i) {
        if (inputs[i] == nullptr) {
            std::cerr << "Error: " << component_name << " - input[" << i << "] not connected" << std::endl;
            return;
        }
        outputs[0] = outputs[0] || (*inputs[i]);
    }
}

