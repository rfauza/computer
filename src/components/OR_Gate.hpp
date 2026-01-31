#pragma once
#include "Component.hpp"

class OR_Gate : public Component
{
public:
    OR_Gate(uint16_t num_bits = 2);
    ~OR_Gate() override;
    void evaluate() override;
};



