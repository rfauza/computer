#pragma once

#include "../components/Component.hpp"

class Part : public Component
{
public:
    Part() = default;
    virtual ~Part();
    void update() override;
};

