#include "Comparator.hpp"
#include <sstream>
#include <iomanip>

Comparator::Comparator(uint16_t num_bits, const std::string& name)
: Device(num_bits, name),
  subtractor(num_bits)
{
    std::ostringstream oss;
    oss << "Comparator 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    // Inputs: A (num_bits) + B (num_bits)
    num_inputs = 2 * num_bits;
    
    // Outputs: EQ, NEQ, LT_U, GT_U, LT_S, GT_S
    num_outputs = 6;
    
    allocate_IO_arrays();
    
    // Create always-high signal for subtractor (always subtract, always enabled)
    always_high = new Signal_Generator("always_high_in_comparator");
    always_high->go_high();
    
    // Wire always_high to subtractor's subtract_enable and output_enable
    subtractor.connect_input(&always_high->get_outputs()[0], 2 * num_bits);      // subtract_enable
    subtractor.connect_input(&always_high->get_outputs()[0], 2 * num_bits + 1);  // output_enable
    
    // Create decoding gates
    not_z = new Inverter(1, "not_z_in_comparator");
    not_c = new Inverter(1, "not_c_in_comparator");
    n_xor_v = new XOR_Gate(2, "n_xor_v_in_comparator");
    not_n_xor_v = new Inverter(1, "not_n_xor_v_in_comparator");
    gt_u_and = new AND_Gate(2, "gt_u_and_in_comparator");
    gt_s_and = new AND_Gate(2, "gt_s_and_in_comparator");
}

Comparator::~Comparator()
{
    delete always_high;
    delete not_z;
    delete not_c;
    delete n_xor_v;
    delete not_n_xor_v;
    delete gt_u_and;
    delete gt_s_and;
}

void Comparator::evaluate()
{
    // Wire A inputs to subtractor
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        subtractor.connect_input(inputs[i], i);
    }
    
    // Wire B inputs to subtractor
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        subtractor.connect_input(inputs[num_bits + i], num_bits + i);
    }
    
    // Evaluate always_high signal and subtractor
    always_high->evaluate();
    subtractor.evaluate();
    
    // Get flags from subtractor (outputs at indices num_bits..num_bits+3)
    bool z_flag = subtractor.get_output(num_bits + 0);  // Z: result is zero
    bool n_flag = subtractor.get_output(num_bits + 1);  // N: negative (MSB of result)
    bool c_flag = subtractor.get_output(num_bits + 2);  // C: carry out
    bool v_flag = subtractor.get_output(num_bits + 3);  // V: signed overflow
    
    // Decode flags into comparison results using gates
    
    // NEQ = !Z
    not_z->connect_input(&subtractor.get_outputs()[num_bits + 0], 0);
    not_z->evaluate();
    
    // LT_U = !C
    not_c->connect_input(&subtractor.get_outputs()[num_bits + 2], 0);
    not_c->evaluate();
    
    // LT_S = N XOR V
    n_xor_v->connect_input(&subtractor.get_outputs()[num_bits + 1], 0);
    n_xor_v->connect_input(&subtractor.get_outputs()[num_bits + 3], 1);
    n_xor_v->evaluate();
    
    // !(N XOR V) for GT_S
    not_n_xor_v->connect_input(&n_xor_v->get_outputs()[0], 0);
    not_n_xor_v->evaluate();
    
    // GT_U = C && !Z
    gt_u_and->connect_input(&subtractor.get_outputs()[num_bits + 2], 0);
    gt_u_and->connect_input(&not_z->get_outputs()[0], 1);
    gt_u_and->evaluate();
    
    // GT_S = !(N XOR V) && !Z
    gt_s_and->connect_input(&not_n_xor_v->get_outputs()[0], 0);
    gt_s_and->connect_input(&not_z->get_outputs()[0], 1);
    gt_s_and->evaluate();
    
    // Assign outputs
    outputs[0] = z_flag;                         // EQ = Z
    outputs[1] = not_z->get_output(0);           // NEQ = !Z
    outputs[2] = not_c->get_output(0);           // LT_U = !C
    outputs[3] = gt_u_and->get_output(0);        // GT_U = C && !Z
    outputs[4] = n_xor_v->get_output(0);         // LT_S = N XOR V
    outputs[5] = gt_s_and->get_output(0);        // GT_S = !(N XOR V) && !Z
}

void Comparator::update()
{
    // Evaluate internal components
    evaluate();
    
    // Signal all downstream components to update
    for (Component* downstream : downstream_components)
    {
        if (downstream)
        {
            downstream->update();
        }
    }
}

