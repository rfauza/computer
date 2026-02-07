#include "Bus.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

Bus::Bus(uint16_t num_bits) : Device(num_bits)
{
    num_outputs = num_bits;
    std::ostringstream oss;
    oss << "Bus 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    initialize_IO_arrays();
}

Bus::~Bus() = default;

void Bus::evaluate()
{
    // If no inputs are connected, output is all zeros
    if (inputs.empty()) {
        for (uint16_t i = 0; i < num_bits; ++i) {
            outputs[i] = false;
        }
        return;
    }
    
    // Bitwise OR all inputs together for each bit position
    for (uint16_t i = 0; i < num_bits; ++i) {
        outputs[i] = false;
        for (bool* input : inputs) {
            outputs[i] = outputs[i] || input[i];
        }
    }
}

void Bus::update()
{
    evaluate();
}

void Bus::attach_input(bool* input_signal)
{
    if (input_signal != nullptr) {
        inputs.push_back(input_signal);
    }
}

void Bus::detach_input(bool* input_signal)
{
    auto it = std::find(inputs.begin(), inputs.end(), input_signal);
    if (it != inputs.end()) {
        inputs.erase(it);
    }
}

