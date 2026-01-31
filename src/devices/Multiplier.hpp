#pragma once
#include "Device.hpp"

class Multiplier : public Device
{
public:
    Multiplier(uint16_t num_bits);
    ~Multiplier() override;
    void update() override;
};

