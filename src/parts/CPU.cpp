#include "CPU.hpp"
#include <sstream>
#include <iomanip>

CPU::CPU(uint16_t num_bits, const std::string& name) : Part(num_bits, name)
{
    std::ostringstream oss;
    oss << "CPU 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty()) {
        oss << " - " << name;
    }
    component_name = oss.str();
}
CPU::~CPU() = default;

void CPU::update()
{
}

