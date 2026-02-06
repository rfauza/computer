#pragma once
#include "Part.hpp"

class Main_Memory : public Part
{
public:
    Main_Memory(uint16_t num_bits);
    ~Main_Memory() override;
    void update() override;
};

