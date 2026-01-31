#include "NOR_Gate.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

NOR_Gate::NOR_Gate(uint16_t num_inputs_param)
{
    num_inputs = num_inputs_param;
    std::ostringstream oss;
    oss << "NOR_Gate 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    num_outputs = 1;
    initialize_IO_arrays();
}

NOR_Gate::~NOR_Gate()
{
}

void NOR_Gate::evaluate()
{
    // NOR: NOT(OR all bits together)
    bool result = false;
    for (uint16_t i = 0; i < num_inputs; ++i) {
        if (inputs[i] == nullptr) {
            std::cerr << "Error: " << component_name << " - input[" << i << "] not connected" << std::endl;
            return;
        }
        result = result || (*inputs[i]);
    }
    outputs[0] = !result;
}

