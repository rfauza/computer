#include "Register.hpp"
#include <sstream>
#include <iomanip>

Register::Register(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Register 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
}
Register::~Register() = default;

void Register::update()
{
}

