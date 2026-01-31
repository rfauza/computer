#pragma once
#include "Component.hpp"

class Buffer : public Component
{
public:
    Buffer(uint16_t num_inputs = 1);
    ~Buffer() override;
    void evaluate() override;
};



