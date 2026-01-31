#pragma once
#include "Part.hpp"

class Main_Memory : public Part
{
public:
    Main_Memory();
    ~Main_Memory() override;
    void update() override;
};

