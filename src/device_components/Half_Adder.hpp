#pragma once
#include "../components/Component.hpp"
#include "../components/NAND_Gate.hpp"
#include "../components/Inverter.hpp"

class Half_Adder : public Component
{
public:
    Half_Adder();
    ~Half_Adder() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;
    
private:
    NAND_Gate nand_gate1{2};
    NAND_Gate nand_gate2{2};
    NAND_Gate nand_gate3{2};
    NAND_Gate nand_gate4{2};
    Inverter inverter1{1};
};

