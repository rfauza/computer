#include "L_Shift.hpp"
#include <sstream>
#include <iomanip>

L_Shift::L_Shift(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "L_Shift 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    
    num_inputs = num_bits;
    num_outputs = num_bits;
    allocate_IO_arrays();
}

L_Shift::~L_Shift() = default;

void L_Shift::update()
{
    // Left shift: output[i] = input[i+1], output[num_bits-1] = 0
    // Since LSB is at index 0, this shifts left (towards lower indices)
    for (uint16_t i = 0; i < num_inputs - 1; ++i)
    {
        if (inputs[i + 1] != nullptr)
        {
            outputs[i] = *inputs[i + 1];
        }
    }
    outputs[num_inputs - 1] = 0;  // MSB becomes 0
    
    // Signal all downstream components to update
    for (Component* downstream : downstream_components)
    {
        if (downstream)
        {
            downstream->update();
        }
    }
}
