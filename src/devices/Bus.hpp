#pragma once
#include "Device.hpp"
#include <vector>

class Bus : public Device
{
public:
    Bus(uint16_t num_bits);
    ~Bus() override;
    
    void evaluate() override;
    void update() override;
    void attach_input(bool* input_signal);
    void detach_input(bool* input_signal);
    
private:
    std::vector<bool*> inputs;
};

