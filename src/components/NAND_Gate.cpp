#include "NAND_Gate.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

NAND_Gate::NAND_Gate(uint16_t num_bits_param)
{
    num_bits = num_bits_param;
    std::ostringstream oss;
    oss << "NAND_Gate 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    num_inputs = num_bits;
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
    for (uint16_t i = 0; i < num_bits; ++i) {
        if (inputs[i] == nullptr) {
            std::cerr << "Error: " << component_name << " - input[" << i << "] not connected" << std::endl;
            return;
        }
        result = result && (*inputs[i]);
    }
    outputs[0] = !result;
}

