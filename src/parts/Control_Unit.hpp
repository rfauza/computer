#pragma once
#include "Part.hpp"

class Control_Unit : public Part
{
public:
    Control_Unit();
    ~Control_Unit() override;
    void update() override;
};

