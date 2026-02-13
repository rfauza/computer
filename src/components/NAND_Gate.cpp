#include "NAND_Gate.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

NAND_Gate::NAND_Gate(uint16_t num_inputs_param, const std::string& name)
    : Component(name)
{
    num_inputs = num_inputs_param;
    // create component name string (include hex address and optional provided name)
    std::ostringstream oss;
    oss << "NAND_Gate 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    num_outputs = 1;
    initialize_IO_arrays();
}

NAND_Gate::~NAND_Gate()
{
}

void NAND_Gate::evaluate()
{
    // NAND: NOT(AND all bits together)
    bool result = true;
    for (uint16_t i = 0; i < num_inputs; ++i) {
        if (inputs[i] == nullptr) {
            std::cerr << "Error: " << component_name << " - input[" << i << "] not connected" << std::endl;
            return;
        }
        result = result && (*inputs[i]);
    }
    outputs[0] = !result;
}

