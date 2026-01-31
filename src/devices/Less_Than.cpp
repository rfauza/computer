#include "Less_Than.hpp"
#include <sstream>
#include <iomanip>

Less_Than::Less_Than(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Less_Than 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
}
Less_Than::~Less_Than() = default;

void Less_Than::update()
{
}

