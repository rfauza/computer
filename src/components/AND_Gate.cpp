#include "AND_Gate.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

AND_Gate::AND_Gate(uint16_t num_inputs_param, const std::string& name)
    : Component(name)
{
    num_inputs = num_inputs_param;
    // create component name string (include hex address and optional provided name)
    std::ostringstream oss;
    oss << "AND_Gate 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    std::cerr << "DEBUG: AND_Gate ctor name_param='" << name << "' actual='" << component_name << "'\n";
    num_outputs = 1;
    initialize_IO_arrays();
}

AND_Gate::~AND_Gate()
{
}

void AND_Gate::evaluate()
{
    // AND all input bits together
    outputs[0] = true;
    for (uint16_t i = 0; i < num_inputs; ++i) {
        if (inputs[i] == nullptr) {
            std::cerr << "Error: " << component_name << " - input[" << i << "] not connected" << std::endl;
            outputs[0] = false; // Set output to false if any input is not connected
            return;
        }
        outputs[0] = outputs[0] && (*inputs[i]);
    }
}

