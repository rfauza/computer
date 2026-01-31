#pragma once
#include "Device.hpp"

class Controller : public Device
{
public:
    Controller(uint16_t num_bits);
    ~Controller() override;
    void update() override;
};

