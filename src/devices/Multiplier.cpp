#include "Multiplier.hpp"
#include <sstream>
#include <iomanip>

Multiplier::Multiplier(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Multiplier 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    
    num_inputs = 2 * num_bits;      // A (num_bits) + B (num_bits)
    num_outputs = 2 * num_bits;     // Product (2*num_bits)
    
    allocate_IO_arrays();
    
    // Allocate AND gate array [num_bits][num_bits]
    and_array = new AND_Gate*[num_bits];
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        and_array[i] = new AND_Gate[num_bits];
    }
    
    // Allocate adders of increasing width [num_bits-1]
    adder_array = new Adder*[num_bits - 1];
    for (uint16_t i = 0; i < num_bits - 1; ++i)
    {
        adder_array[i] = new Adder(num_bits + i + 1);
    }
    
    // Allocate zeros for shift padding (max needed is num_bits-1)
    zeros = new Signal_Generator*[num_bits];
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        zeros[i] = new Signal_Generator();
        zeros[i]->go_low();
    }
    
    // Wire the adder cascade
    // Output bit 0 comes directly from first AND gate
    outputs[0] = &and_array[0][0].get_outputs()[0];
    
    // First adder: add row 0 (shifted) + row 1 (shifted)
    // A input: {and_array[0][1], and_array[0][2], ..., and_array[0][num_bits-1], 0}
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        adder_array[0]->connect_input(&and_array[0][i + 1].get_outputs()[0], i);
    }
    adder_array[0]->connect_input(&zeros[0]->get_outputs()[0], num_bits);
    
    // B input: {0, and_array[1][0], and_array[1][1], ..., and_array[1][num_bits-1]}
    adder_array[0]->connect_input(&zeros[1]->get_outputs()[0], num_bits + 1);
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        adder_array[0]->connect_input(&and_array[1][i].get_outputs()[0], num_bits + 1 + i + 1);
    }
    
    // Output bit 1 is LSB of first adder
    outputs[1] = &adder_array[0]->get_outputs()[0];
    
    // Subsequent adders
    for (uint16_t stage = 1; stage < num_bits - 1; ++stage)
    {
        uint16_t adder_width = num_bits + stage + 1;
        
        // A input: previous adder output (shifted: bits 1 to width-1, then 0)
        for (uint16_t i = 0; i < adder_width - 1; ++i)
        {
            adder_array[stage]->connect_input(&adder_array[stage - 1]->get_outputs()[i + 1], i);
        }
        adder_array[stage]->connect_input(&zeros[stage * 2]->get_outputs()[0], adder_width - 1);
        
        // B input: {0, and_array[stage+1][0..num_bits-1], zeros...}
        adder_array[stage]->connect_input(&zeros[stage * 2 + 1]->get_outputs()[0], adder_width);
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            adder_array[stage]->connect_input(&and_array[stage + 1][i].get_outputs()[0], adder_width + i + 1);
        }
        // Pad remaining with zeros
        for (uint16_t i = num_bits + 1; i < adder_width; ++i)
        {
            adder_array[stage]->connect_input(&zeros[stage * 2 + 1]->get_outputs()[0], adder_width + i);
        }
        
        // Output bit stage+1 is LSB of this adder
        outputs[stage + 1] = &adder_array[stage]->get_outputs()[0];
    }
    
    // Final outputs: upper bits from last adder
    uint16_t last_adder = num_bits - 2;
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        outputs[num_bits + i] = &adder_array[last_adder]->get_outputs()[i + 1];
    }
}

Multiplier::~Multiplier()
{
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        delete[] and_array[i];
        delete zeros[i];
    }
    delete[] and_array;
    delete[] zeros;
    
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
            and_array[row][input_index].connect_input(inputs[input_index], 0);
        }
    }
    // Connect B inputs (num_bits to 2*num_bits-1) to second input of each AND gate row
    else if (input_index < 2 * num_bits)
    {
        uint16_t b_bit = input_index - num_bits;
        for (uint16_t col = 0; col < num_bits; ++col)
        {
            and_array[b_bit][col].connect_input(inputs[input_index], 1);
        }
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
            and_array[row][col].evaluate();
        }
    }
    
    // Evaluate adder cascade
    for (uint16_t i = 0; i < num_bits - 1; ++i)
    {
        adder_array[i]->evaluate();
    }
}

void Multiplier::update()
{
    evaluate();
    
    for (auto& component : downstream_components)
    {
        component->update();
    }
}

