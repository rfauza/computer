#pragma once
#include "Component.hpp"

class Inverter : public Component
{
public:
    Inverter(uint16_t num_inputs = 1);
    ~Inverter() override;
    void evaluate() override;
};



