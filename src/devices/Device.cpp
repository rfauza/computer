#include "Device.hpp"

Device::Device(uint16_t num_bits, const std::string& name) : Component(name), num_bits(num_bits)
{
}

Device::~Device()
{
}

void Device::update()
{
}

