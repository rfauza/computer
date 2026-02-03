#include "Program_Memory.hpp"
#include <sstream>
#include <iomanip>

Program_Memory::Program_Memory(uint16_t num_bits)
    : Part(num_bits),
      decoder(num_bits),
    input_bus(static_cast<uint16_t>(4 * num_bits)),
    output_bus(static_cast<uint16_t>(4 * num_bits))
{
    std::ostringstream oss;
    oss << "Program_Memory 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    
    if (num_bits == 0)
        num_bits = 1;
    
    num_addresses = static_cast<uint16_t>(1u << num_bits); // 2^num_bits
    data_bus_bits = static_cast<uint16_t>(4 * num_bits); // Opcode, C, A, B
    
    num_inputs = static_cast<uint16_t>(5 * num_bits + 2); // addr select, opcode, C, A, B, WE, RE
    num_outputs = data_bus_bits;
    allocate_IO_arrays();
    
    // Attach the data inputs to the input bus (skip the first [num_bits] address selector bits)
    input_bus.attach_input(inputs[num_bits]);

    // Allocate register arrays and select-gates for each address
    for (uint16_t i = 0; i < 4; ++i)
    {
        registers[i] = new Register*[num_addresses];
    }
    write_selects = new AND_Gate*[num_addresses];
    read_selects = new AND_Gate*[num_addresses];
    
    // for each address, create select gates and 4 memory registers (opcode, C, A, B)
    for (uint16_t addr = 0; addr < num_addresses; ++addr)
    {
        write_selects[addr] = new AND_Gate(2);
        read_selects[addr] = new AND_Gate(2);
        
        // Connect decoder output to select gates (input 0)
        write_selects[addr]->connect_input(&decoder.get_outputs()[addr], 0);
        read_selects[addr]->connect_input(&decoder.get_outputs()[addr], 0);
        
        // create 4 registers (opcode, C, A, B) for each address
        for (uint16_t reg_index = 0; reg_index < registers_per_address; ++reg_index)
        {
            Register* reg = new Register(num_bits);
            // registers[reg_index][addr] = reg;
            registers[addr][reg_index] = reg;
            
            // Wire register data inputs to the input bus
            uint16_t bus_offset = static_cast<uint16_t>(reg_index * num_bits); // offset by numbits for each register
            // connect each bit
            for (uint16_t bit = 0; bit < num_bits; ++bit)
            {
                reg->connect_input(&input_bus.get_outputs()[bus_offset + bit], bit);
            }
            
            // Wire shared write/read enables (ANDed with address selector)
            reg->connect_input(&write_selects[addr]->get_outputs()[0], num_bits);
            reg->connect_input(&read_selects[addr]->get_outputs()[0], num_bits + 1);
            
            // Attach register outputs to shared output_bus
            output_bus.attach_input(reg->get_outputs());
        }
    }
}

Program_Memory::~Program_Memory()
{
    for (uint16_t i = 0; i < 4; ++i)
    {
        delete[] registers[i];
    }

    for (uint16_t i = 0; i < 4; ++i)
    {
        delete[] registers[i];
    }
    delete[] write_selects;
    delete[] read_selects;
    
    // output_bus destroyed automatically
}

bool Program_Memory::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Connect incoming output to this device's input array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Internal Connections:
    // Connect address bits to decoder
    if (input_index < num_bits)
    {
        // Address bits -> decoder inputs
        return decoder.connect_input(inputs[input_index], input_index);
    }
    // Do not connect data bus bits directly since Bus has this device's input array as one of its inputs in constructor
    else if (input_index < static_cast<uint16_t>(num_bits + data_bus_bits))
    {
        // Input_bus bits are read directly from this device's input
        return true;
    }
    // Connect write enable
    else if (input_index == static_cast<uint16_t>(5 * num_bits))
    {
        // Write Enable -> all write select gates (input 1)
        for (uint16_t i = 0; i < num_addresses; ++i)
        {
            write_selects[i]->connect_input(inputs[input_index], 1);
        }
        return true;
    }
    // Connect read enable
    else if (input_index == static_cast<uint16_t>(5 * num_bits + 1))
    {
        // Read Enable -> all read select gates (input 1)
        for (uint16_t i = 0; i < num_addresses; ++i)
        {
            read_selects[i]->connect_input(inputs[input_index], 1);
        }
        return true;
    }
    
    return false;
}

void Program_Memory::evaluate()
{
    input_bus.evaluate();
    decoder.evaluate();
    
    for (uint16_t i = 0; i < num_addresses; ++i)
    {
        write_selects[i]->evaluate();
        read_selects[i]->evaluate();
    }

    for (uint16_t addr = 0; addr < num_addresses; ++addr)
    {
        for (uint16_t i = 0; i < 4; ++i)
        {
            registers[addr][i]->evaluate();
        }
    }
    
    output_bus.evaluate();
    
    // Copy output bus to outputs array
    for (uint16_t bit = 0; bit < data_bus_bits; ++bit)
    {
        outputs[bit] = output_bus.get_output(bit);
    }
}

void Program_Memory::update()
{
    evaluate();
    
    for (Component* downstream : downstream_components)
    {
        if (downstream)
        {
            downstream->update();
        }
    }
}
