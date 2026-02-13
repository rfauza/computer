#include "Buffer.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

Buffer::Buffer(uint16_t num_inputs_param, const std::string& name)
    : Component(name)
{
    num_inputs = num_inputs_param;
    std::ostringstream oss;
    oss << "Buffer 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    num_outputs = num_inputs;
    initialize_IO_arrays();
}

Buffer::~Buffer()
{
}

void Buffer::evaluate()
{
    // Pass through: Output[i] = Input[i]
    for (uint16_t i = 0; i < num_inputs; ++i) {
        if (inputs[i] == nullptr) {
            std::cerr << "Error: " << component_name << " - input[" << i << "] not connected" << std::endl;
            return;
        }
        outputs[i] = *inputs[i];
    }
}

