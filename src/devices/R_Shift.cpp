#include "R_Shift.hpp"
#include <sstream>
#include <iomanip>

R_Shift::R_Shift(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "R_Shift 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
}
R_Shift::~R_Shift() = default;

void R_Shift::update()
{
    // Right shift: output[i] = input[i+1], output[num_bits-1] = 0
    // Since LSB is at index 0, this shifts right (towards higher indices)
    outputs[0] = 0;  // LSB becomes 0
    for (uint16_t i = 1; i < num_inputs; ++i)
    {
        outputs[i] = *inputs[i - 1];
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

