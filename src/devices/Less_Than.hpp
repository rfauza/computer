#pragma once
#include "Device.hpp"

class Less_Than : public Device
{
public:
    Less_Than(uint16_t num_bits);
    ~Less_Than() override;
    void update() override;
};

