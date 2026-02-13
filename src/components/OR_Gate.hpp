#pragma once
#include "Component.hpp"

class OR_Gate : public Component
{
public:
    OR_Gate(uint16_t num_inputs = 2, const std::string& name = "");
    ~OR_Gate() override;
    void evaluate() override;
};



