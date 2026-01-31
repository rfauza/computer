#pragma once
#include "Part.hpp"

class CPU : public Part
{
public:
    CPU();
    ~CPU() override;
    void update() override;
};

