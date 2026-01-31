#pragma once
#include "Component.hpp"

class AND_Gate : public Component
{
public:
    AND_Gate(uint16_t num_inputs = 2);
    ~AND_Gate() override;
    void evaluate() override;
};


