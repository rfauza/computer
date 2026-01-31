#include "AND_Gate.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

AND_Gate::AND_Gate(uint16_t num_inputs_param)
{
    num_inputs = num_inputs_param;
    std::ostringstream oss;
    oss << "AND_Gate 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
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
            return;
        }
        outputs[0] = outputs[0] && (*inputs[i]);
    }
}

