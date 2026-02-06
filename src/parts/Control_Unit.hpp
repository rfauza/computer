#pragma once
#include "Part.hpp"

class Control_Unit : public Part
{
public:
    Control_Unit(uint16_t num_bits);
    ~Control_Unit() override;
    void update() override;
};

