#pragma once
#include "Part.hpp"

class Arithmetic_Unit : public Part
{
public:
    Arithmetic_Unit();
    ~Arithmetic_Unit() override;
    void update() override;
};

