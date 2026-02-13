#include "Signal_Generator.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

Signal_Generator::Signal_Generator(const std::string& name)
    : Component(name)
{
    std::ostringstream oss;
    oss << "Signal_Generator 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    num_inputs = 0;
    num_outputs = 1;
    initialize_IO_arrays();
}

Signal_Generator::~Signal_Generator()
{
}

void Signal_Generator::evaluate()
{
    // Signal generator has no inputs to evaluate
    // Output is set directly via go_high() and go_low()
}

void Signal_Generator::go_high()
{
    for (uint16_t i = 0; i < num_outputs; ++i) {
        outputs[i] = true;
    }
}

void Signal_Generator::go_low()
{
    for (uint16_t i = 0; i < num_outputs; ++i) {
        outputs[i] = false;
    }
}

bool Signal_Generator::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    std::cerr << "Error: " << component_name << " does not accept any inputs" << std::endl;
    return false;
}
