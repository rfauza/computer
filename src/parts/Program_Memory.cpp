#include "Program_Memory.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

Program_Memory::Program_Memory(uint16_t decoder_bits, uint16_t data_bits, const std::string& name)
    : Part(static_cast<uint16_t>(decoder_bits + 4 * data_bits + 2), name),
      decoder(decoder_bits)
{
    // make component name
    std::ostringstream oss;
    oss << "Program_Memory 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty()) {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    if (decoder_bits == 0)
        decoder_bits = 1;
    if (data_bits == 0)
        data_bits = 1;
    
    this->decoder_bits = decoder_bits;
    this->data_bits = data_bits;
    
    num_addresses = static_cast<uint16_t>(1u << decoder_bits); // 2^decoder_bits
    
    num_inputs = static_cast<uint16_t>(decoder_bits + 4 * data_bits + 2); // addr select, opcode, C, A, B, WE, RE
    num_outputs = static_cast<uint16_t>(4 * data_bits);
    allocate_IO_arrays();

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
        std::ostringstream ws_name, rs_name;
        ws_name << "write_select_" << addr << "_in_program_memory";
        rs_name << "read_select_" << addr << "_in_program_memory";
        write_selects[addr] = new AND_Gate(2, ws_name.str());
        read_selects[addr] = new AND_Gate(2, rs_name.str());
        
        // Connect decoder output to select gates (input 0)
        write_selects[addr]->connect_input(&decoder.get_outputs()[addr], 0);
        read_selects[addr]->connect_input(&decoder.get_outputs()[addr], 0);
        
        // create 4 registers (opcode, C, A, B) for each address
        for (uint16_t reg_index = 0; reg_index < registers_per_address; ++reg_index)
        {
            std::ostringstream reg_name;
            reg_name << "register_" << reg_index << "_addr_" << addr << "_in_program_memory";
            Register* reg = new Register(data_bits, reg_name.str());
            registers[reg_index][addr] = reg;
        }
    }
}

Program_Memory::~Program_Memory()
{
    // Delete all registers
    for (uint16_t i = 0; i < 4; ++i)
    {
        for (uint16_t j = 0; j < num_addresses; ++j)
        {
            delete registers[i][j];
        }
        delete[] registers[i];
    }
    
    // Delete all select gates
    for (uint16_t i = 0; i < num_addresses; ++i)
    {
        delete write_selects[i];
        delete read_selects[i];
    }
    delete[] write_selects;
    delete[] read_selects;
}

bool Program_Memory::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Connect incoming output to this device's input array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Internal Connections:
    // Connect address bits to decoder
    if (input_index < decoder_bits)
    {
        // Address bits -> decoder inputs
        return decoder.connect_input(inputs[input_index], input_index);
    }
    // Connect data bits to ALL registers
    else if (input_index < static_cast<uint16_t>(decoder_bits + 4 * data_bits))
    {
        uint16_t data_bit_index = static_cast<uint16_t>(input_index - decoder_bits);
        uint16_t reg_index = data_bit_index / data_bits;  // which register (opcode=0, C=1, A=2, B=3)
        uint16_t bit_within_reg = data_bit_index % data_bits;  // which bit within that register
        
        // Connect this input to all copies of this register across all addresses
        for (uint16_t addr = 0; addr < num_addresses; ++addr)
        {
            registers[reg_index][addr]->connect_input(inputs[input_index], bit_within_reg);
        }
        return true;
    }
    // Connect write enable
    else if (input_index == static_cast<uint16_t>(decoder_bits + 4 * data_bits))
    {
        // Write Enable -> all write select gates (input 1)
        for (uint16_t i = 0; i < num_addresses; ++i)
        {
            write_selects[i]->connect_input(inputs[input_index], 1);
            
            // Connect write_select output to all registers at this address
            for (uint16_t reg = 0; reg < registers_per_address; ++reg)
            {
                registers[reg][i]->connect_input(&write_selects[i]->get_outputs()[0], data_bits);
            }
        }
        return true;
    }
    // Connect read enable
    else if (input_index == static_cast<uint16_t>(decoder_bits + 4 * data_bits + 1))
    {
        // Read Enable -> all read select gates (input 1)
        for (uint16_t i = 0; i < num_addresses; ++i)
        {
            read_selects[i]->connect_input(inputs[input_index], 1);
            
            // Connect read_select output to all registers at this address
            for (uint16_t reg = 0; reg < registers_per_address; ++reg)
            {
                registers[reg][i]->connect_input(&read_selects[i]->get_outputs()[0], data_bits + 1);
            }
        }
        return true;
    }
    
    return false;
}

void Program_Memory::evaluate()
{
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
            registers[i][addr]->evaluate();
        }
    }
    
    // Manually OR outputs from all addresses
    // For each bit position in output (16 bits = 4 registers × 4 bits)
    for (uint16_t bit = 0; bit < static_cast<uint16_t>(4 * data_bits); ++bit)
    {
        uint16_t reg_index = bit / data_bits;  // Which register (0-3)
        uint16_t bit_in_reg = bit % data_bits; // Which bit in that register (0-3)
        
        outputs[bit] = false;
        for (uint16_t addr = 0; addr < num_addresses; ++addr)
        {
            outputs[bit] = outputs[bit] || registers[reg_index][addr]->get_output(bit_in_reg);
        }
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
