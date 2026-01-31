#pragma once
#include "Component.hpp"

class NAND_Gate : public Component
{
public:
    NAND_Gate(uint16_t num_inputs = 2);
    ~NAND_Gate() override;
    void evaluate() override;
};



