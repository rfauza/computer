#pragma once
#include "Device.hpp"

class Greater_Than : public Device
{
public:
    Greater_Than(uint16_t num_bits);
    ~Greater_Than() override;
    void update() override;
};

