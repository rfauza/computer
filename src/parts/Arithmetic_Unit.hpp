#pragma once
#include "Part.hpp"

class Arithmetic_Unit : public Part
{
public:
    Arithmetic_Unit(uint16_t num_bits);
    ~Arithmetic_Unit() override;
    void update() override;
};

