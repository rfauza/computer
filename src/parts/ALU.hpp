#pragma once
#include "Part.hpp"

class ALU : public Part
{
public:
    ALU();
    ~ALU() override;
    void update() override;
};

