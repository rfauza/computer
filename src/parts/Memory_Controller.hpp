#pragma once
#include "Part.hpp"

class Memory_Controller : public Part
{
public:
    Memory_Controller();
    ~Memory_Controller() override;
    void update() override;
};

