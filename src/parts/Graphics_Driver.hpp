#pragma once
#include "Part.hpp"

class Graphics_Driver : public Part
{
public:
    Graphics_Driver(uint16_t num_bits, const std::string& name = "");
    ~Graphics_Driver() override;
    void update() override;
};

