#pragma once
#include "Component.hpp"

class Inverter : public Component
{
public:
    Inverter(uint16_t num_inputs = 1, const std::string& name = "");
    ~Inverter() override;
    void evaluate() override;
};



