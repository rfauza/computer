#include "Flip_Flop.hpp"
#include <sstream>
#include <iomanip>

Flip_Flop::Flip_Flop(const std::string& name)
        : Component(name),
            inverter_set(1, name.empty() ? std::string("inverter_set_in_flip_flop") : name + "_inverter_set"),
            inverter_reset(1, name.empty() ? std::string("inverter_reset_in_flip_flop") : name + "_inverter_reset"),
      nand_gate_1(2, name.empty() ? std::string("nand_gate_1_in_flip_flop") : name + "_nand_gate_1"),
      nand_gate_2(2, name.empty() ? std::string("nand_gate_2_in_flip_flop") : name + "_nand_gate_2")
{
    std::ostringstream oss;
    oss << "Flip_Flop 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    num_inputs = 2;  // [Set, Reset]
    num_outputs = 1; // [Q (output)]
    
    allocate_IO_arrays();
    
    // Invert Set and Reset inputs for active-high SR latch
    // Set passes through inverter_set → nand_gate_1 input 0
    // Reset passes through inverter_reset → nand_gate_2 input 1
    // Cross-coupled NAND gates: outputs feed back to each other's inputs
    inverter_set.connect_output(&nand_gate_1, 0, 0);
    inverter_reset.connect_output(&nand_gate_2, 0, 1);
    nand_gate_1.connect_output(&nand_gate_2, 0, 0);
    nand_gate_2.connect_output(&nand_gate_1, 0, 1);
    
    // Initialize to reset state (Q=0, Q_not=1) by setting internal gate outputs
    nand_gate_1.get_outputs()[0] = false;  // Q = 0
    nand_gate_2.get_outputs()[0] = true;   // Q_not = 1
    outputs[0] = false;  // Output reflects Q
}

Flip_Flop::~Flip_Flop()
{
}

bool Flip_Flop::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route inputs through inverters then to NAND gates
    if (input_index == 0)
    {
        // input[0] (Set) → inverter_set input → nand_gate_1
        return inverter_set.connect_input(inputs[0], 0);
    }
    else if (input_index == 1)
    {
        // input[1] (Reset) → inverter_reset input → nand_gate_2
        return inverter_reset.connect_input(inputs[1], 0);
    }
    
    return true;
}

void Flip_Flop::evaluate()
{
    // Evaluate in order: inverters → NAND gates → feedback settles
    inverter_set.evaluate();
    inverter_reset.evaluate();
    nand_gate_1.evaluate();
    nand_gate_2.evaluate();
    nand_gate_1.evaluate();  // Second pass to let feedback settle
    nand_gate_2.evaluate();
    
    // Output is Q from nand_gate_1
    outputs[0] = nand_gate_1.get_output(0);
}

void Flip_Flop::update()
{
    // Phase 2: update internal components so feedback/latches settle
    inverter_set.update();
    inverter_reset.update();
    nand_gate_1.update();
    nand_gate_2.update();
    // extra passes to let feedback settle
    nand_gate_1.update();
    nand_gate_2.update();

    // Refresh output from internal gate
    outputs[0] = nand_gate_1.get_output(0);

    // Do not automatically propagate updates from leaf device internals;
    // higher-level parts/controllers orchestrate update propagation to avoid recursion.
}
