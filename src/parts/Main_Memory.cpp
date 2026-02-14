#include "Main_Memory.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

Main_Memory::Main_Memory(uint16_t address_bits, uint16_t data_bits, const std::string& name)
    : Part(static_cast<uint16_t>(3 * address_bits + data_bits + 3), name),
      decoder_a(address_bits),
      decoder_b(address_bits),
      decoder_c(address_bits)
{
    // make component name
    std::ostringstream oss;
    oss << "Main_Memory 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty()) {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    if (address_bits == 0)
        address_bits = 1;
    if (data_bits == 0)
        data_bits = 1;
    
    this->address_bits = address_bits;
    this->data_bits = data_bits;
    
    num_addresses = static_cast<uint16_t>(1u << address_bits); // 2^address_bits
    
    num_inputs = static_cast<uint16_t>(3 * address_bits + data_bits + 3); // 3 addrs, data, WE, RE_A, RE_B
    num_outputs = static_cast<uint16_t>(2 * data_bits); // 2 read outputs
    allocate_IO_arrays();

    // Allocate register array and select-gates for each address
    registers = new Register*[num_addresses];
    write_selects = new AND_Gate*[num_addresses];
    read_selects_a = new AND_Gate*[num_addresses];
    read_selects_b = new AND_Gate*[num_addresses];
    
    // for each address, create select gates and one memory register
    for (uint16_t addr = 0; addr < num_addresses; ++addr)
    {
        std::ostringstream ws_name, rsa_name, rsb_name;
        ws_name << "write_select_" << addr << "_in_main_memory";
        rsa_name << "read_select_a_" << addr << "_in_main_memory";
        rsb_name << "read_select_b_" << addr << "_in_main_memory";
        write_selects[addr] = new AND_Gate(2, ws_name.str());
        read_selects_a[addr] = new AND_Gate(2, rsa_name.str());
        read_selects_b[addr] = new AND_Gate(2, rsb_name.str());
        
        // Connect decoder outputs to select gates (input 0)
        write_selects[addr]->connect_input(&decoder_c.get_outputs()[addr], 0);
        read_selects_a[addr]->connect_input(&decoder_a.get_outputs()[addr], 0);
        read_selects_b[addr]->connect_input(&decoder_b.get_outputs()[addr], 0);
        
        // create register for this address (with hierarchical name)
        {
            std::ostringstream reg_name;
            reg_name << "register_addr_" << addr << "_in_main_memory";
            registers[addr] = new Register(data_bits, reg_name.str());
        }
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
        delete read_selects_a[i];
        delete read_selects_b[i];
    }
    delete[] write_selects;
    delete[] read_selects_a;
    delete[] read_selects_b;
}

bool Main_Memory::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Connect incoming output to this device's input array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Internal Connections:
    // Connect address A bits to decoder_a
    if (input_index < address_bits)
    {
        return decoder_a.connect_input(inputs[input_index], input_index);
    }
    // Connect address B bits to decoder_b
    else if (input_index < static_cast<uint16_t>(2 * address_bits))
    {
        uint16_t b_bit_index = static_cast<uint16_t>(input_index - address_bits);
        return decoder_b.connect_input(inputs[input_index], b_bit_index);
    }
    // Connect address C bits to decoder_c
    else if (input_index < static_cast<uint16_t>(3 * address_bits))
    {
        uint16_t c_bit_index = static_cast<uint16_t>(input_index - 2 * address_bits);
        return decoder_c.connect_input(inputs[input_index], c_bit_index);
    }
    // Connect write data bits to ALL registers
    else if (input_index < static_cast<uint16_t>(3 * address_bits + data_bits))
    {
        uint16_t data_bit_index = static_cast<uint16_t>(input_index - 3 * address_bits);
        
        // Connect this input to all registers
        for (uint16_t addr = 0; addr < num_addresses; ++addr)
        {
            registers[addr]->connect_input(inputs[input_index], data_bit_index);
        }
        return true;
    }
    // Connect write enable
    else if (input_index == static_cast<uint16_t>(3 * address_bits + data_bits))
    {
        // Write Enable -> all write select gates (input 1)
        for (uint16_t i = 0; i < num_addresses; ++i)
        {
            write_selects[i]->connect_input(inputs[input_index], 1);
            
            // Connect write_select output to register write enable
            registers[i]->connect_input(&write_selects[i]->get_outputs()[0], data_bits);
        }
        return true;
    }
    // Connect read enable A
    else if (input_index == static_cast<uint16_t>(3 * address_bits + data_bits + 1))
    {
        // Read Enable A -> all read select A gates (input 1) and all register read enables
        // Registers must have RE=1 to output for dual-port simultaneous reads
        for (uint16_t i = 0; i < num_addresses; ++i)
        {
            read_selects_a[i]->connect_input(inputs[input_index], 1);
            // Connect RE_A directly to all registers (not through select gates)
            registers[i]->connect_input(inputs[input_index], data_bits + 1);
        }
        return true;
    }
    // Connect read enable B (just for select gates, registers share RE_A)
    else if (input_index == static_cast<uint16_t>(3 * address_bits + data_bits + 2))
    {
        // Read Enable B -> all read select B gates (input 1)
        for (uint16_t i = 0; i < num_addresses; ++i)
        {
            read_selects_b[i]->connect_input(inputs[input_index], 1);
        }
        return true;
    }
    
    return false;
}

void Main_Memory::evaluate()
{
    // Evaluate all three decoders
    decoder_a.evaluate();
    decoder_b.evaluate();
    decoder_c.evaluate();
    
    // (debug prints removed)
    
    // Evaluate all select gates
    for (uint16_t i = 0; i < num_addresses; ++i)
    {
        write_selects[i]->evaluate();
        read_selects_a[i]->evaluate();
        read_selects_b[i]->evaluate();
    }

    // Evaluate all registers
    for (uint16_t addr = 0; addr < num_addresses; ++addr)
    {
        registers[addr]->evaluate();
    }
    
    // Manually OR outputs for port A (bits 0..data_bits-1)
    // Use read_select_a to gate which register's output appears on port A
    for (uint16_t bit = 0; bit < data_bits; ++bit)
    {
        outputs[bit] = false;
        for (uint16_t addr = 0; addr < num_addresses; ++addr)
        {
            // read_select_a[addr] = decoder_a[addr] & RE_A (only one active)
            if (read_selects_a[addr]->get_outputs()[0] && registers[addr]->get_output(bit))
            {
                outputs[bit] = true;
                break;  // Only one address is selected, so we can break early
            }
        }
    }
    
    // Manually OR outputs for port B (bits data_bits..2*data_bits-1)  
    // Use read_select_b to gate which register's output appears on port B
    for (uint16_t bit = 0; bit < data_bits; ++bit)
    {
        outputs[data_bits + bit] = false;
        for (uint16_t addr = 0; addr < num_addresses; ++addr)
        {
            // read_select_b[addr] = decoder_b[addr] & RE_B (only one active)
            if (read_selects_b[addr]->get_outputs()[0] && registers[addr]->get_output(bit))
            {
                outputs[data_bits + bit] = true;
                break;  // Only one address is selected, so we can break early
            }
        }
    }
}

void Main_Memory::update()
{
    // Phase 2 of clock cycle: Only latch storage elements (registers)
    // Do NOT call evaluate() here - that already happened in Phase 1
    
    for (uint16_t addr = 0; addr < num_addresses; ++addr)
    {
        registers[addr]->update();
    }
    
    for (Component* downstream : downstream_components)
    {
        if (downstream)
        {
            downstream->update();
        }
    }
}

