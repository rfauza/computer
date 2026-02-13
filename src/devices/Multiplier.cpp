#include "Multiplier.hpp"
#include <sstream>
#include <iomanip>

Multiplier::Multiplier(uint16_t num_bits, const std::string& name) : Device(num_bits, name)
{
    std::ostringstream oss;
    oss << "Multiplier 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    num_inputs = 2 * num_bits + 1;      // A (num_bits) + B (num_bits) + output_enable
    num_outputs = 2 * num_bits;         // Product (2*num_bits)
    
    allocate_IO_arrays();
    
    // Initialize all outputs to false
    for (uint16_t i = 0; i < num_outputs; ++i)
    {
        outputs[i] = false;
    }
    
    // Allocate AND gate array [num_bits][num_bits]
    and_array = new AND_Gate**[num_bits];
    std::ostringstream and_name;
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        and_array[i] = new AND_Gate*[num_bits];
        for (uint16_t j = 0; j < num_bits; ++j)
        {
            and_name.str("");
            and_name.clear();
            and_name << "and_gate_" << i << "_" << j << "_in_multiplier";
            and_array[i][j] = new AND_Gate(2, and_name.str());
        }
    }
    
    // Allocate adders of increasing width [num_bits-1]
    adder_array = new Adder*[num_bits - 1];
    std::ostringstream adder_name;
    for (uint16_t i = 0; i < num_bits - 1; ++i)
    {
        adder_name.str("");
        adder_name.clear();
        adder_name << "adder_" << i << "_in_multiplier";
        adder_array[i] = new Adder(num_bits + i + 1, adder_name.str());
    }
    
    // Allocate zeros for shift padding (max needed is num_bits)
    zeros = new Signal_Generator*[num_bits];
    std::ostringstream zero_name;
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        zero_name.str("");
        zero_name.clear();
        zero_name << "zero_" << i << "_in_multiplier";
        zeros[i] = new Signal_Generator(zero_name.str());
        zeros[i]->go_low();
    }
    
    // Allocate AND gates for output gating
    output_AND_gates = new AND_Gate*[2 * num_bits];
    std::ostringstream output_and_name;
    for (uint16_t i = 0; i < 2 * num_bits; ++i)
    {
        output_and_name.str("");
        output_and_name.clear();
        output_and_name << "output_and_" << i << "_in_multiplier";
        output_AND_gates[i] = new AND_Gate(2, output_and_name.str());
    }
    
    // Initialize output_enable pointer
    output_enable = nullptr;
    // First adder: add row 0 (shifted) + row 1 (shifted)
    uint16_t first_adder_width = num_bits + 1;
    // A input: {and_array[0][1], and_array[0][2], ..., and_array[0][num_bits-1], 0}
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        if (i + 1 < num_bits)
            adder_array[0]->connect_input(&and_array[0][i + 1]->get_outputs()[0], i);
        else
            adder_array[0]->connect_input(&zeros[0]->get_outputs()[0], i);
    }
    adder_array[0]->connect_input(&zeros[0]->get_outputs()[0], num_bits);
    
    // B input: {and_array[1][0], and_array[1][1], ..., and_array[1][num_bits-1], 0}
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        adder_array[0]->connect_input(&and_array[1][i]->get_outputs()[0], first_adder_width + i);
    }
    adder_array[0]->connect_input(&zeros[1]->get_outputs()[0], first_adder_width + num_bits);
    
    // Subsequent adders
    for (uint16_t stage = 1; stage < num_bits - 1; ++stage)
    {
        uint16_t adder_width = num_bits + stage + 1;
        
        // A input: previous adder output (shifted: bits 1 to width-1, then 0)
        for (uint16_t i = 0; i < adder_width - 1; ++i)
        {
            if (i + 1 < adder_array[stage - 1]->get_num_outputs())
                adder_array[stage]->connect_input(&adder_array[stage - 1]->get_outputs()[i + 1], i);
            else
                adder_array[stage]->connect_input(&zeros[stage]->get_outputs()[0], i);
        }
        adder_array[stage]->connect_input(&zeros[stage]->get_outputs()[0], adder_width - 1);
        
        // B input: {and_array[stage+1][0..num_bits-1], zeros...}
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            adder_array[stage]->connect_input(&and_array[stage + 1][i]->get_outputs()[0], adder_width + i);
        }
        // Pad remaining B inputs with zeros
        for (uint16_t i = 0; i < adder_width - num_bits; ++i)
        {
            adder_array[stage]->connect_input(&zeros[stage]->get_outputs()[0], adder_width + num_bits + i);
        }
    }
}

Multiplier::~Multiplier()
{
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        for (uint16_t j = 0; j < num_bits; ++j)
        {
            delete and_array[i][j];
        }
        delete[] and_array[i];
        delete zeros[i];
    }
    delete[] and_array;
    delete[] zeros;
    for (uint16_t i = 0; i < 2 * num_bits; ++i)
    {
        delete output_AND_gates[i];
    }
    delete[] output_AND_gates;
    
    for (uint16_t i = 0; i < num_bits - 1; ++i)
    {
        delete adder_array[i];
    }
    delete[] adder_array;
}

bool Multiplier::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Connect A inputs (0 to num_bits-1) to first input of each AND gate column
    if (input_index < num_bits)
    {
        for (uint16_t row = 0; row < num_bits; ++row)
        {
            and_array[row][input_index]->connect_input(inputs[input_index], 0);
        }
    }
    // Connect B inputs (num_bits to 2*num_bits-1) to second input of each AND gate row
    else if (input_index < 2 * num_bits)
    {
        uint16_t b_bit = input_index - num_bits;
        for (uint16_t col = 0; col < num_bits; ++col)
        {
            and_array[b_bit][col]->connect_input(inputs[input_index], 1);
        }
    }
    // Connect output_enable (2*num_bits) to all output AND gates input 1
    else if (input_index == 2 * num_bits)
    {
        output_enable = inputs[input_index];
        bool result = true;
        for (uint16_t i = 0; i < 2 * num_bits; ++i)
        {
            result &= output_AND_gates[i]->connect_input(output_enable, 1);
        }
        return result;
    }
    
    return true;
}

void Multiplier::evaluate()
{
    // Evaluate all AND gates for partial products
    for (uint16_t row = 0; row < num_bits; ++row)
    {
        for (uint16_t col = 0; col < num_bits; ++col)
        {
            and_array[row][col]->evaluate();
        }
    }
    
    // Evaluate adder cascade
    for (uint16_t i = 0; i < num_bits - 1; ++i)
    {
        adder_array[i]->evaluate();
    }
    
    // Compute intermediate results
    bool* intermediate = new bool[2 * num_bits];
    
    // Intermediate bit 0 comes from and_array[0][0]
    intermediate[0] = and_array[0][0]->get_output(0);
    
    // Intermediate bits 1 through num_bits-1 come from first adder's LSB
    for (uint16_t i = 0; i < num_bits - 1; ++i)
    {
        intermediate[i + 1] = adder_array[i]->get_output(0);
    }
    
    // Final intermediate bits from last adder
    uint16_t last_adder = num_bits - 2;
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        if (i + 1 < adder_array[last_adder]->get_num_outputs())
            intermediate[num_bits + i] = adder_array[last_adder]->get_output(i + 1);
    }
    
    // Gate outputs through AND gates with output_enable
    for (uint16_t i = 0; i < 2 * num_bits; ++i)
    {
        // If output_enable is connected, connect intermediate to AND gate input 0
        if (output_enable != nullptr)
        {
            output_AND_gates[i]->connect_input(&intermediate[i], 0);
            output_AND_gates[i]->evaluate();
            outputs[i] = output_AND_gates[i]->get_output(0);
        }
        else
        {
            // If output_enable not connected, pass through directly
            outputs[i] = intermediate[i];
        }
    }
    
    delete[] intermediate;
}

void Multiplier::update()
{
    evaluate();
    
    for (auto& component : downstream_components)
    {
        component->update();
    }
}

