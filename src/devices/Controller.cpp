#include "Controller.hpp"
#include <sstream>
#include <iomanip>

Controller::Controller(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Controller 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
}
Controller::~Controller() = default;

void Controller::update()
{
}

