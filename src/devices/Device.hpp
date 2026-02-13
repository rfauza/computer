#pragma once

#include "../components/Component.hpp"
#include "../components/Buffer.hpp"
#include "../components/Inverter.hpp"
#include "../components/AND_Gate.hpp"
#include "../components/OR_Gate.hpp"
#include "../components/NAND_Gate.hpp"
#include "../components/NOR_Gate.hpp"
#include "../components/XOR_Gate.hpp"
#include "../components/Signal_Generator.hpp"
#include "../device_components/Half_Adder.hpp"
#include "../device_components/Full_Adder.hpp"
#include "../device_components/Full_Adder_Subtractor.hpp"
#include "../device_components/Flip_Flop.hpp"

class Device : public Component
{
public:
    Device(uint16_t num_bits, const std::string& name = "");
    virtual ~Device();
    void update() override;
    
protected:
    uint16_t num_bits;
};
