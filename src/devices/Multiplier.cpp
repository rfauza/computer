#include "Multiplier.hpp"
#include <sstream>
#include <iomanip>

Multiplier::Multiplier(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Multiplier 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
}
Multiplier::~Multiplier() = default;

void Multiplier::update()
{
}

