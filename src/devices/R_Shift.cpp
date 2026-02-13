#include "R_Shift.hpp"
#include <sstream>
#include <iomanip>

R_Shift::R_Shift(uint16_t num_bits, const std::string& name) : Device(num_bits, name)
{
    std::ostringstream oss;
    oss << "R_Shift 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    num_inputs = num_bits;
    num_outputs = num_bits;
    allocate_IO_arrays();
}

R_Shift::~R_Shift() = default;

void R_Shift::update()
{
    // Right shift: output[i] = input[i-1], output[0] = 0
    // Since LSB is at index 0, this shifts right (towards higher indices)
    outputs[0] = 0;  // LSB becomes 0
    for (uint16_t i = 1; i < num_inputs; ++i)
    {
        if (inputs[i - 1] != nullptr)
        {
            outputs[i] = *inputs[i - 1];
        }
    }
    
    // Signal all downstream components to update
    for (Component* downstream : downstream_components)
    {
        if (downstream)
        {
            downstream->update();
        }
    }
}
