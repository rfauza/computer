#pragma once
#include "Part.hpp"

class Memory_Controller : public Part
{
public:
    Memory_Controller(uint16_t num_bits);
    ~Memory_Controller() override;
    void update() override;
};

