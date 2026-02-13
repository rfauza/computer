#pragma once
#include "Component.hpp"

class NOR_Gate : public Component
{
public:
    NOR_Gate(uint16_t num_inputs = 2, const std::string& name = "");
    ~NOR_Gate() override;
    void evaluate() override;
};



