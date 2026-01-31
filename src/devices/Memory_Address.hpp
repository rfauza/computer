#pragma once
#include "Device.hpp"

class Memory_Address : public Device
{
public:
    Memory_Address(uint16_t num_bits);
    ~Memory_Address() override;
    void update() override;
};

