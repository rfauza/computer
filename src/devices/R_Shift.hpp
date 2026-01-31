#pragma once
#include "Device.hpp"

class R_Shift : public Device
{
public:
    R_Shift(uint16_t num_bits);
    ~R_Shift() override;
    void update() override;
};

