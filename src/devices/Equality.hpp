#pragma once
#include "Device.hpp"

class Equality : public Device
{
public:
    Equality(uint16_t num_bits);
    ~Equality() override;
    void update() override;
};

