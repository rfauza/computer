#include "Equality.hpp"
#include <sstream>
#include <iomanip>

Equality::Equality(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Equality 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
}
Equality::~Equality() = default;

void Equality::update()
{
}

