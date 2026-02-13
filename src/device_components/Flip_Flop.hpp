#pragma once
#include "../components/Component.hpp"
#include "../components/NAND_Gate.hpp"
#include "../components/Inverter.hpp"

/**
 * @brief SR Latch (Set-Reset flip-flop) implemented with NAND gates
 * 
 * A basic SR latch with two inputs (Set, Reset) and one output (Q).
 * - Set (input[0]): Sets the output Q to 1 (active high)
 * - Reset (input[1]): Resets the output Q to 0 (active high)
 * - Output (output[0]): The stored state
 */
class Flip_Flop : public Component
{
public:
    Flip_Flop(const std::string& name = "");
    ~Flip_Flop() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;

private:
    Inverter inverter_set;      // Inverts Set input for NAND latch
    Inverter inverter_reset;    // Inverts Reset input for NAND latch
    NAND_Gate nand_gate_1;      // Cross-coupled NAND gate 1
    NAND_Gate nand_gate_2;      // Cross-coupled NAND gate 2
};

