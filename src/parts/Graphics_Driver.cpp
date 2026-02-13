#include "Graphics_Driver.hpp"
#include <sstream>
#include <iomanip>

Graphics_Driver::Graphics_Driver(uint16_t num_bits, const std::string& name) : Part(num_bits, name)
{
    std::ostringstream oss;
    oss << "Graphics_Driver 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty()) {
        oss << " - " << name;
    }
    component_name = oss.str();
}
Graphics_Driver::~Graphics_Driver() = default;

void Graphics_Driver::update()
{
}

