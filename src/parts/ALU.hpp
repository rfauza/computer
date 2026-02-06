#pragma once
#include "Part.hpp"

class ALU : public Part
{
public:
    ALU(uint16_t num_bits);
    ~ALU() override;
    void update() override;
};

