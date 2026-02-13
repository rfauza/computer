#include "Memory_Controller.hpp"
#include <sstream>
#include <iomanip>

Memory_Controller::Memory_Controller(uint16_t num_bits, const std::string& name) : Part(num_bits, name)
{
    std::ostringstream oss;
    oss << "Memory_Controller 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty()) {
        oss << " - " << name;
    }
    component_name = oss.str();
}
Memory_Controller::~Memory_Controller() = default;

void Memory_Controller::update()
{
}

