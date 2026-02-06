#pragma once
#include "Part.hpp"

class CPU : public Part
{
public:
    CPU(uint16_t num_bits);
    ~CPU() override;
    void update() override;
};

