#pragma once
#include "Device.hpp"

class Divider : public Device
{
public:
    Divider(uint16_t num_bits);
    ~Divider() override;
    void update() override;
};

