#pragma once
#include "Part.hpp"

class Graphics_Driver : public Part
{
public:
    Graphics_Driver();
    ~Graphics_Driver() override;
    void update() override;
};

