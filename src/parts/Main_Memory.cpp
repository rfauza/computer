#include "Main_Memory.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

Main_Memory::Main_Memory(uint16_t address_bits, uint16_t data_bits)
    : Part(static_cast<uint16_t>(address_bits + data_bits + 2)),
      decoder(address_bits)
{
    // make component name
    std::ostringstream oss;
    oss << "Main_Memory 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    
    if (address_bits == 0)
        address_bits = 1;
    if (data_bits == 0)
        data_bits = 1;
    
    this->address_bits = address_bits;
    this->data_bits = data_bits;
    
    num_addresses = static_cast<uint16_t>(1u << address_bits); // 2^address_bits
    
    num_inputs = static_cast<uint16_t>(address_bits + data_bits + 2); // addr select, data, WE, RE
    num_outputs = data_bits;
    allocate_IO_arrays();

    // Allocate register array and select-gates for each address
    registers = new Register*[num_addresses];
    write_selects = new AND_Gate*[num_addresses];
    read_selects = new AND_Gate*[num_addresses];
    
    // for each address, create select gates and one memory register
    for (uint16_t addr = 0; addr < num_addresses; ++addr)
    {
        write_selects[addr] = new AND_Gate(2);
        read_selects[addr] = new AND_Gate(2);
        
        // Connect decoder output to select gates (input 0)
        write_selects[addr]->connect_input(&decoder.get_outputs()[addr], 0);
        read_selects[addr]->connect_input(&decoder.get_outputs()[addr], 0);
        
        // create register for this address
        registers[addr] = new Register(data_bits);
    }
}

Main_Memory::~Main_Memory()
{
    // Delete all registers
    for (uint16_t i = 0; i < num_addresses; ++i)
    {
        delete registers[i];
    }
    delete[] registers;
    
    // Delete all select gates
    for (uint16_t i = 0; i < num_addresses; ++i)
    {
        delete write_selects[i];
        delete read_selects[i];
    }
    delete[] write_selects;
    delete[] read_selects;
}

bool Main_Memory::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Connect incoming output to this device's input array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Internal Connections:
    // Connect address bits to decoder
    if (input_index < address_bits)
    {
        // Address bits -> decoder inputs
        return decoder.connect_input(inputs[input_index], input_index);
    }
    // Connect data bits to ALL registers
    else if (input_index < static_cast<uint16_t>(address_bits + data_bits))
    {
        uint16_t data_bit_index = static_cast<uint16_t>(input_index - address_bits);
        
        // Connect this input to all registers
        for (uint16_t addr = 0; addr < num_addresses; ++addr)
        {
            registers[addr]->connect_input(inputs[input_index], data_bit_index);
        }
        return true;
    }
    // Connect write enable
    else if (input_index == static_cast<uint16_t>(address_bits + data_bits))
    {
        // Write Enable -> all write select gates (input 1)
        for (uint16_t i = 0; i < num_addresses; ++i)
        {
            write_selects[i]->connect_input(inputs[input_index], 1);
            
            // Connect write_select output to register at this address
            registers[i]->connect_input(&write_selects[i]->get_outputs()[0], data_bits);
        }
        return true;
    }
    // Connect read enable
    else if (input_index == static_cast<uint16_t>(address_bits + data_bits + 1))
    {
        // Read Enable -> all read select gates (input 1)
        for (uint16_t i = 0; i < num_addresses; ++i)
        {
            read_selects[i]->connect_input(inputs[input_index], 1);
            
            // Connect read_select output to register at this address
            registers[i]->connect_input(&read_selects[i]->get_outputs()[0], data_bits + 1);
        }
        return true;
    }
    
    return false;
}

void Main_Memory::evaluate()
{
    decoder.evaluate();
    
    for (uint16_t i = 0; i < num_addresses; ++i)
    {
        write_selects[i]->evaluate();
        read_selects[i]->evaluate();
    }

    for (uint16_t addr = 0; addr < num_addresses; ++addr)
    {
        registers[addr]->evaluate();
    }
    
    // Manually OR outputs from all addresses
    for (uint16_t bit = 0; bit < data_bits; ++bit)
    {
        outputs[bit] = false;
        for (uint16_t addr = 0; addr < num_addresses; ++addr)
        {
            outputs[bit] = outputs[bit] || registers[addr]->get_output(bit);
        }
    }
}

void Main_Memory::update()
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

