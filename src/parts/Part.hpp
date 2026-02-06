#pragma once

#include "../devices/Device.hpp"

class Part : public Device
{
public:
    Part(uint16_t num_bits);
    virtual ~Part();
    void update() override;
};

