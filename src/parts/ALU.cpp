#include "ALU.hpp"
#include <sstream>
#include <iomanip>

ALU::ALU(uint16_t num_bits, const std::string& name) : Part(num_bits, name)
{
    // generate component name string
    std::ostringstream oss;
    oss << "ALU 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty()) {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    // Inputs: data_a (num_bits) + data_b (num_bits) + 11 enables
    num_inputs = (2 * num_bits) + 11;
    // Outputs: result (num_bits) + comparison flags (6)
    num_outputs = num_bits + 6;
    
    allocate_IO_arrays();
    
    // Create sub-units
    arithmetic_unit = new Arithmetic_Unit(num_bits, "arithmetic_unit_in_alu");
    logic_unit = new Logic_Unit(num_bits, "logic_unit_in_alu");
    comparator = new Comparator(num_bits, "comparator_in_alu");
    
    // Note: Comparator will receive A and B inputs directly in connect_input
    // No wiring needed here - will be done per-input in connect_input method
    
    // Initialize enable pointers
    add_enable = nullptr;
    sub_enable = nullptr;
    inc_enable = nullptr;
    dec_enable = nullptr;
    mul_enable = nullptr;
    and_enable = nullptr;
    or_enable = nullptr;
    xor_enable = nullptr;
    not_enable = nullptr;
    r_shift_enable = nullptr;
    l_shift_enable = nullptr;
}

ALU::~ALU()
{
    delete arithmetic_unit;
    delete logic_unit;
    delete comparator;
}

bool ALU::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route inputs: [data_a (num_bits) | data_b (num_bits) | 11 enable signals]
    if (input_index < num_bits)
    {
        // Data A input: route to arithmetic, logic, and comparator
        bool result = true;
        result &= arithmetic_unit->connect_input(inputs[input_index], input_index);
        result &= logic_unit->connect_input(inputs[input_index], input_index);
        result &= comparator->connect_input(inputs[input_index], input_index);
        return result;
    }
    else if (input_index < 2 * num_bits)
    {
        // Data B input: route to arithmetic, logic, and comparator
        bool result = true;
        result &= arithmetic_unit->connect_input(inputs[input_index], input_index);
        result &= logic_unit->connect_input(inputs[input_index], input_index);
        result &= comparator->connect_input(inputs[input_index], input_index);
        return result;
    }
    else if (input_index == 2 * num_bits + 0)
    {
        // add_enable
        add_enable = inputs[input_index];
        arithmetic_unit->connect_input(inputs[input_index], 2 * num_bits + 0);
    }
    else if (input_index == 2 * num_bits + 1)
    {
        // sub_enable
        sub_enable = inputs[input_index];
        arithmetic_unit->connect_input(inputs[input_index], 2 * num_bits + 1);
    }
    else if (input_index == 2 * num_bits + 2)
    {
        // inc_enable
        inc_enable = inputs[input_index];
        arithmetic_unit->connect_input(inputs[input_index], 2 * num_bits + 2);
    }
    else if (input_index == 2 * num_bits + 3)
    {
        // dec_enable
        dec_enable = inputs[input_index];
        arithmetic_unit->connect_input(inputs[input_index], 2 * num_bits + 3);
    }
    else if (input_index == 2 * num_bits + 4)
    {
        // mul_enable
        mul_enable = inputs[input_index];
        arithmetic_unit->connect_input(inputs[input_index], 2 * num_bits + 4);
    }
    else if (input_index == 2 * num_bits + 5)
    {
        // and_enable
        and_enable = inputs[input_index];
        logic_unit->connect_input(inputs[input_index], 2 * num_bits + 0);
    }
    else if (input_index == 2 * num_bits + 6)
    {
        // or_enable
        or_enable = inputs[input_index];
        logic_unit->connect_input(inputs[input_index], 2 * num_bits + 1);
    }
    else if (input_index == 2 * num_bits + 7)
    {
        // xor_enable
        xor_enable = inputs[input_index];
        logic_unit->connect_input(inputs[input_index], 2 * num_bits + 2);
    }
    else if (input_index == 2 * num_bits + 8)
    {
        // not_enable
        not_enable = inputs[input_index];
        logic_unit->connect_input(inputs[input_index], 2 * num_bits + 3);
    }
    else if (input_index == 2 * num_bits + 9)
    {
        // r_shift_enable
        r_shift_enable = inputs[input_index];
        logic_unit->connect_input(inputs[input_index], 2 * num_bits + 4);
    }
    else if (input_index == 2 * num_bits + 10)
    {
        // l_shift_enable
        l_shift_enable = inputs[input_index];
        logic_unit->connect_input(inputs[input_index], 2 * num_bits + 5);
    }
    
    return true;
}

void ALU::evaluate()
{
    // Always evaluate comparator (for comparison flags)
    comparator->evaluate();
    //Check which unit's operation is enabled and evaluate it
    // Arithmetic operations: add, sub, inc, dec, mul (indices 0-4)
    if ((add_enable && *add_enable) || (sub_enable && *sub_enable) ||
        (inc_enable && *inc_enable) || (dec_enable && *dec_enable) ||
        (mul_enable && *mul_enable))
    {
        arithmetic_unit->evaluate();
        // Copy arithmetic unit result to ALU output
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = arithmetic_unit->get_output(i);
        }
        
        // Evaluate comparator (uses AU's flag outputs which are now updated)
        comparator->evaluate();
    }
    // Logic operations: and, or, xor, not, r_shift, l_shift (indices 5-10)
    else if ((and_enable && *and_enable) || (or_enable && *or_enable) ||
             (xor_enable && *xor_enable) || (not_enable && *not_enable) ||
             (r_shift_enable && *r_shift_enable) || (l_shift_enable && *l_shift_enable))
    {
        logic_unit->evaluate();
        // Copy logic unit result to ALU output
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = logic_unit->get_output(i);
        }
        
        // Still need to evaluate AU for flags, then evaluate comparator
        arithmetic_unit->evaluate();
        comparator->evaluate();
    }
    else
    {
        // No operation enabled - output all zeros
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = false;
        }
        
        // Still evaluate AU and comparator for flags
        arithmetic_unit->evaluate();
        comparator->evaluate();
    }
    // Copy comparison flags to outputs[num_bits..num_bits+5]
    for (uint16_t i = 0; i < 6; ++i)
    {
        outputs[num_bits + i] = comparator->get_output(i);
    }
}

void ALU::update()
{
    // Evaluate this component
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

void ALU::print_comparator_io() const
{
    if (comparator)
        comparator->print_io();
}

