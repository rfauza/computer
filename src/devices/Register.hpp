#pragma once
#include "Device.hpp"

class Register : public Device
{
public:
    Register(uint16_t num_bits);
    ~Register() override;
    void update() override;
};

