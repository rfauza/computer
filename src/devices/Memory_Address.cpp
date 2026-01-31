#include "Memory_Address.hpp"
#include <sstream>
#include <iomanip>

Memory_Address::Memory_Address(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Memory_Address 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
}
Memory_Address::~Memory_Address() = default;

void Memory_Address::update()
{
}

