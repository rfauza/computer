#pragma once
#include "Part.hpp"

class Logic_Unit : public Part
{
public:
    Logic_Unit();
    ~Logic_Unit() override;
    void update() override;
};

