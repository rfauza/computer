#include "Arithmetic_Unit.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

Arithmetic_Unit::Arithmetic_Unit(uint16_t num_bits, const std::string& name) 
: Part(num_bits, name), 
    adder_subtractor(num_bits),
    multiplier(num_bits),
    adder_output_enable_or(nullptr),
    adder_subtract_enable_or(nullptr),
    add_or_sub_or(nullptr),
    inc_or_dec_or(nullptr),
    constant_bits(nullptr),
    data_b_gates(nullptr),
    constant_one_gates(nullptr),
    b_input_or_gates(nullptr)
{
    // create component name string
    std::ostringstream oss;
    oss << "Arithmetic_Unit 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty()) {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    // Inputs: data_a (num_bits) + data_b (num_bits) + add_enable (1) + sub_enable (1) + 
    // increment_enable (1) + decrement_enable (1) + mul_enable (1)
    num_inputs = (2 * num_bits) + 5;
    num_outputs = num_bits;  // result only, no flags
    
    allocate_IO_arrays();    
    // Create the OR gates on the heap
    adder_output_enable_or = new OR_Gate(4, "adder_output_enable_or_in_arithmetic_unit");
    adder_subtract_enable_or = new OR_Gate(2, "adder_subtract_enable_or_in_arithmetic_unit");
    add_or_sub_or = new OR_Gate(2, "add_or_sub_or_in_arithmetic_unit");  // OR(add_enable, sub_enable)
    inc_or_dec_or = new OR_Gate(2, "inc_or_dec_or_in_arithmetic_unit");  // OR(inc_enable, dec_enable)
    
    // Create constant 1 signal generators for INC/DEC (array of bits, only LSB=1)
    constant_bits = new Signal_Generator[num_bits];
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        if (i == 0)  // LSB = 1
            constant_bits[i].go_high();
        else         // rest = 0
            constant_bits[i].go_low();
    }
    
    // Allocate gate arrays for B input gating (num_bits gates each)
    data_b_gates = new AND_Gate[num_bits];
    constant_one_gates = new AND_Gate[num_bits];
    b_input_or_gates = new OR_Gate[num_bits];
    
    // Wire gating logic for each bit of B input
    // B_to_adder[i] = (data_b[i] AND (add OR sub)) OR (1 AND (inc OR dec))
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        // data_b_gates[i]: data_b[i] AND (add OR sub)
        add_or_sub_or->connect_output(&data_b_gates[i], 0, 1);
        
        // constant_one_gates[i]: constant_bits[i] AND (inc OR dec)
        constant_bits[i].connect_output(&constant_one_gates[i], 0, 0);
        inc_or_dec_or->connect_output(&constant_one_gates[i], 0, 1);
        
        // b_input_or_gates[i]: OR the two paths
        data_b_gates[i].connect_output(&b_input_or_gates[i], 0, 0);
        constant_one_gates[i].connect_output(&b_input_or_gates[i], 0, 1);
        
        // Connect final OR output to adder_subtractor's B input
        b_input_or_gates[i].connect_output(&adder_subtractor, 0, num_bits + i);
    }
    
    // Wire OR output to adder_subtractor's output_enable input
    adder_output_enable_or->connect_output(&adder_subtractor, 0, 2 * num_bits + 1);
    
    // Wire subtract OR output to adder_subtractor's subtract_enable input
    adder_subtract_enable_or->connect_output(&adder_subtractor, 0, 2 * num_bits);
    
    // Set up aliases
    data_a = inputs;
    data_b = &inputs[num_bits];
    add_enable = nullptr;
    sub_enable = nullptr;
    inc_enable = nullptr;
    dec_enable = nullptr;
    mul_enable = nullptr;
}

Arithmetic_Unit::~Arithmetic_Unit()
{
    delete adder_output_enable_or;
    delete adder_subtract_enable_or;
    delete add_or_sub_or;
    delete inc_or_dec_or;
    delete[] constant_bits;
    delete[] data_b_gates;
    delete[] constant_one_gates;
    delete[] b_input_or_gates;
}

bool Arithmetic_Unit::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route inputs: [data_a (num_bits), data_b (num_bits),  add_enable (1), sub_enable (1), 
    // inc_enable (1), dec_enable (1), mul_enable (1)]
    if (input_index < num_bits)
    {
        // Data A input: route to all arithmetic devices
        bool result = true;
        result &= adder_subtractor.connect_input(inputs[input_index], input_index);
        result &= multiplier.connect_input(inputs[input_index], input_index);
        return result;
    }
    else if (input_index < 2 * num_bits)
    {
        // Data B input: route to gating logic AND multiplier
        bool result = true;
        uint16_t b_bit = input_index - num_bits;
        
        // Connect to data_b_gates (will be AND'ed with add_or_sub_or)
        result &= data_b_gates[b_bit].connect_input(inputs[input_index], 0);
        
        // Connect to multiplier
        result &= multiplier.connect_input(inputs[input_index], input_index);
        return result;
    }
    else if (input_index == 2 * num_bits)
    {
        // add_enable - route to OR gates
        add_enable = inputs[input_index];
        adder_output_enable_or->connect_input(inputs[input_index], 0);
        add_or_sub_or->connect_input(inputs[input_index], 0);
    }
    else if (input_index == 2 * num_bits + 1)
    {
        // sub_enable - route to OR gates
        sub_enable = inputs[input_index];
        adder_output_enable_or->connect_input(inputs[input_index], 1);
        adder_subtract_enable_or->connect_input(inputs[input_index], 0);
        add_or_sub_or->connect_input(inputs[input_index], 1);
    }
    else if (input_index == 2 * num_bits + 2)
    {
        // inc_enable - route to OR gates
        inc_enable = inputs[input_index];
        adder_output_enable_or->connect_input(inputs[input_index], 2);
        inc_or_dec_or->connect_input(inputs[input_index], 0);
    }
    else if (input_index == 2 * num_bits + 3)
    {
        // dec_enable - route to OR gates
        dec_enable = inputs[input_index];
        adder_output_enable_or->connect_input(inputs[input_index], 3);
        adder_subtract_enable_or->connect_input(inputs[input_index], 1);
        inc_or_dec_or->connect_input(inputs[input_index], 1);
    }
    else if (input_index == 2 * num_bits + 4)
    {
        // mul_enable - store locally and connect to multiplier's output_enable
        mul_enable = inputs[input_index];
        multiplier.connect_input(inputs[input_index], 2 * num_bits);
    }
    
    return true;
}

void Arithmetic_Unit::evaluate()
{
    // Evaluate the operation enable OR gates
    adder_output_enable_or->evaluate();
    adder_subtract_enable_or->evaluate();
    add_or_sub_or->evaluate();
    inc_or_dec_or->evaluate();
    
    // Evaluate B input gating logic
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        data_b_gates[i].evaluate();
        constant_one_gates[i].evaluate();
        b_input_or_gates[i].evaluate();
    }
    
    // Evaluate adder_subtractor for selected operation result
    adder_subtractor.evaluate();
    
    // Determine which operation is enabled and copy result from selected device
    if (add_enable && *add_enable)
    {
        // Result from adder
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = adder_subtractor.get_output(i);
        }
    }
    else if (sub_enable && *sub_enable)
    {
        // Result from adder
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = adder_subtractor.get_output(i);
        }
    }
    else if (inc_enable && *inc_enable)
    {
        // Result from adder
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = adder_subtractor.get_output(i);
        }
    }
    else if (dec_enable && *dec_enable)
    {
        // Result from adder
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = adder_subtractor.get_output(i);
        }
    }
    else if (mul_enable && *mul_enable)
    {
        multiplier.evaluate();
        // Result from multiplier (2*num_bits outputs, we take lower num_bits)
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = multiplier.get_output(i);
        }
    }
    else
    {
        // No operation enabled, set result to zero
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = false;
        }
    }
}

void Arithmetic_Unit::update()
{
    evaluate();
    
    // Propagate to downstream components
    for (Component* downstream : downstream_components)
    {
        if (downstream)
        {
            downstream->update();
        }
    }
}

void Arithmetic_Unit::print_adder_inputs() const
{
    std::cout << "Adder_Subtractor inputs: ";
    adder_subtractor.print_io();
}

void Arithmetic_Unit::print_multiplier_io() const
{
    std::cout << "Multiplier IO: ";
    multiplier.print_io();
}
