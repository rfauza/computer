#include "Divider.hpp"
#include <sstream>
#include <iomanip>

Divider::Divider(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Divider 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
}
Divider::~Divider() = default;

void Divider::update()
{
}

