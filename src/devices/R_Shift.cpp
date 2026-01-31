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
}

