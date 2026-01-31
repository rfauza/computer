#include "Greater_Than.hpp"
#include <sstream>
#include <iomanip>

Greater_Than::Greater_Than(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Greater_Than 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
}
Greater_Than::~Greater_Than() = default;

void Greater_Than::update()
{
}

