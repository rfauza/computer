#include "CPU.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

CPU::CPU(uint16_t num_bits, const std::string& opcode_string, const std::string& name) 
    : Part(num_bits, name), low_signal(nullptr)
{
    std::ostringstream oss;
    oss << "CPU 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    opcode_bits = num_bits;  // Opcode size equals data width
    num_decoder_outputs = static_cast<uint16_t>(1u << opcode_bits);
    
    // Create Control Unit and ALU
    control_unit = new Control_Unit(num_bits, "control_unit_in_cpu");
    alu = new ALU(num_bits, "alu_in_cpu");
    
    // Parse the opcode specification
    parse_opcodes(opcode_string);
    
    // Create OR gates for combining decoder outputs to ALU enables
    // Each OR gate will have as many inputs as there are opcodes mapping to that operation
    add_enable_or = new OR_Gate(num_decoder_outputs, "add_enable_or_in_cpu");
    sub_enable_or = new OR_Gate(num_decoder_outputs, "sub_enable_or_in_cpu");
    inc_enable_or = new OR_Gate(num_decoder_outputs, "inc_enable_or_in_cpu");
    dec_enable_or = new OR_Gate(num_decoder_outputs, "dec_enable_or_in_cpu");
    mul_enable_or = new OR_Gate(num_decoder_outputs, "mul_enable_or_in_cpu");
    and_enable_or = new OR_Gate(num_decoder_outputs, "and_enable_or_in_cpu");
    or_enable_or = new OR_Gate(num_decoder_outputs, "or_enable_or_in_cpu");
    xor_enable_or = new OR_Gate(num_decoder_outputs, "xor_enable_or_in_cpu");
    not_enable_or = new OR_Gate(num_decoder_outputs, "not_enable_or_in_cpu");
    rsh_enable_or = new OR_Gate(num_decoder_outputs, "rsh_enable_or_in_cpu");
    lsh_enable_or = new OR_Gate(num_decoder_outputs, "lsh_enable_or_in_cpu");
    
    // Wire decoder outputs to ALU enables
    wire_decoder_to_alu();
    
    // Connect ALU comparator flags to Control Unit flag register
    const bool** flag_ptrs = new const bool*[6];
    for (uint16_t i = 0; i < 6; ++i)
    {
        flag_ptrs[i] = &alu->get_outputs()[num_bits + i];
    }
    control_unit->connect_comparator_flags(flag_ptrs, 6);
    delete[] flag_ptrs;
}

CPU::~CPU()
{
    delete control_unit;
    delete alu;
    delete add_enable_or;
    delete sub_enable_or;
    delete inc_enable_or;
    delete dec_enable_or;
    delete mul_enable_or;
    delete and_enable_or;
    delete or_enable_or;
    delete xor_enable_or;
    delete not_enable_or;
    delete rsh_enable_or;
    delete lsh_enable_or;
    delete low_signal;
}

void CPU::parse_opcodes(const std::string& opcode_string)
{
    std::istringstream iss(opcode_string);
    std::string line;
    
    while (std::getline(iss, line))
    {
        // Skip empty lines
        if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos)
            continue;
        
        // Parse line format: "0000 HALT" or "0001 ADD"
        std::istringstream line_stream(line);
        std::string opcode_str, operation_name;
        
        if (!(line_stream >> opcode_str >> operation_name))
        {
            std::cerr << "Warning: Could not parse opcode line: " << line << std::endl;
            continue;
        }
        
        // Convert binary string to integer
        uint16_t opcode = 0;
        for (char c : opcode_str)
        {
            opcode = (opcode << 1) | (c == '1' ? 1 : 0);
        }
        
        // Store bidirectional mapping
        operation_to_opcode[operation_name] = opcode;
        opcode_to_operation[opcode] = operation_name;
    }
}

void CPU::wire_decoder_to_alu()
{
    bool* decoder_outputs = control_unit->get_decoder_outputs();
    
    // Initialize all OR gate inputs to low (so unconnected inputs don't cause errors)
    low_signal = new Signal_Generator("low_signal_in_cpu");
    low_signal->go_low();
    low_signal->evaluate();
    
    for (uint16_t i = 0; i < num_decoder_outputs; ++i)
    {
        add_enable_or->connect_input(&low_signal->get_outputs()[0], i);
        sub_enable_or->connect_input(&low_signal->get_outputs()[0], i);
        inc_enable_or->connect_input(&low_signal->get_outputs()[0], i);
        dec_enable_or->connect_input(&low_signal->get_outputs()[0], i);
        mul_enable_or->connect_input(&low_signal->get_outputs()[0], i);
        and_enable_or->connect_input(&low_signal->get_outputs()[0], i);
        or_enable_or->connect_input(&low_signal->get_outputs()[0], i);
        xor_enable_or->connect_input(&low_signal->get_outputs()[0], i);
        not_enable_or->connect_input(&low_signal->get_outputs()[0], i);
        rsh_enable_or->connect_input(&low_signal->get_outputs()[0], i);
        lsh_enable_or->connect_input(&low_signal->get_outputs()[0], i);
    }
    
    // For each decoder output, connect it to the appropriate OR gate based on the operation
    for (uint16_t opcode = 0; opcode < num_decoder_outputs; ++opcode)
    {
        auto it = opcode_to_operation.find(opcode);
        if (it == opcode_to_operation.end())
        {
            // No operation defined for this opcode - connect to all OR gates as low
            // (OR gates will ignore low inputs)
            continue;
        }
        
        const std::string& operation = it->second;
        
    // Connect decoder output to the appropriate enable OR gate
        if (operation == "ADD")
            add_enable_or->connect_input(&decoder_outputs[opcode], opcode);
        else if (operation == "SUB")
            sub_enable_or->connect_input(&decoder_outputs[opcode], opcode);
        else if (operation == "INC")
            inc_enable_or->connect_input(&decoder_outputs[opcode], opcode);
        else if (operation == "DEC")
            dec_enable_or->connect_input(&decoder_outputs[opcode], opcode);
        else if (operation == "MUL")
            mul_enable_or->connect_input(&decoder_outputs[opcode], opcode);
        else if (operation == "AND")
            and_enable_or->connect_input(&decoder_outputs[opcode], opcode);
        else if (operation == "OR")
            or_enable_or->connect_input(&decoder_outputs[opcode], opcode);
        else if (operation == "XOR")
            xor_enable_or->connect_input(&decoder_outputs[opcode], opcode);
        else if (operation == "NOT")
            not_enable_or->connect_input(&decoder_outputs[opcode], opcode);
        else if (operation == "RSH")
            rsh_enable_or->connect_input(&decoder_outputs[opcode], opcode);
        else if (operation == "LSH")
            lsh_enable_or->connect_input(&decoder_outputs[opcode], opcode);
        // Control operations (HALT, NOP, JMP, CMP, etc.) don't map to ALU enables
    }
    
    // Connect OR gate outputs to ALU enables
    // ALU input layout: [data_a, data_b, enables...]
    uint16_t enable_offset = 2 * num_bits;  // After data_a and data_b
    alu->connect_input(&add_enable_or->get_outputs()[0], enable_offset + 0);
    alu->connect_input(&sub_enable_or->get_outputs()[0], enable_offset + 1);
    alu->connect_input(&inc_enable_or->get_outputs()[0], enable_offset + 2);
    alu->connect_input(&dec_enable_or->get_outputs()[0], enable_offset + 3);
    alu->connect_input(&mul_enable_or->get_outputs()[0], enable_offset + 4);
    alu->connect_input(&and_enable_or->get_outputs()[0], enable_offset + 5);
    alu->connect_input(&or_enable_or->get_outputs()[0], enable_offset + 6);
    alu->connect_input(&xor_enable_or->get_outputs()[0], enable_offset + 7);
    alu->connect_input(&not_enable_or->get_outputs()[0], enable_offset + 8);
    alu->connect_input(&rsh_enable_or->get_outputs()[0], enable_offset + 9);
    alu->connect_input(&lsh_enable_or->get_outputs()[0], enable_offset + 10);
}

bool CPU::connect_program_memory(const bool* const* pm_opcode_outputs, bool** pm_address_inputs)
{
    if (!pm_opcode_outputs)
        return false;
    
    // Connect PC to PM address (optional - only if address inputs provided)
    if (pm_address_inputs)
        control_unit->connect_pc_to_pm_address(pm_address_inputs, 0);
    
    // Connect PM opcode to CU decoder
    control_unit->connect_opcode_input(pm_opcode_outputs, opcode_bits);
    
    return true;
}

bool CPU::connect_data_inputs(
    const bool* const* data_c_outputs,
    const bool* const* data_a_outputs,
    const bool* const* data_b_outputs)
{
    if (!data_a_outputs || !data_b_outputs)
        return false;
    
    // Connect data inputs to ALU
    // ALU input layout: [data_a[0..num_bits-1], data_b[0..num_bits-1], enables...]
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        alu->connect_input(data_a_outputs[i], i);
        alu->connect_input(data_b_outputs[i], num_bits + i);
    }
    
    return true;
}

bool* CPU::get_result_outputs() const
{
    return alu->get_outputs();
}

bool* CPU::get_pc_outputs() const
{
    return control_unit->get_pc_outputs();
}

int CPU::get_opcode_for_operation(const std::string& operation_name) const
{
    auto it = operation_to_opcode.find(operation_name);
    if (it != operation_to_opcode.end())
        return static_cast<int>(it->second);
    return -1;
}

void CPU::clock_tick()
{
    control_unit->clock_tick();
}

void CPU::evaluate()
{
    // Evaluate in dependency order
    
    // 1. Control Unit (PC, decoder)
    control_unit->evaluate();
    
    // 2. OR gates combining decoder outputs
    add_enable_or->evaluate();
    sub_enable_or->evaluate();
    inc_enable_or->evaluate();
    dec_enable_or->evaluate();
    mul_enable_or->evaluate();
    and_enable_or->evaluate();
    or_enable_or->evaluate();
    xor_enable_or->evaluate();
    not_enable_or->evaluate();
    rsh_enable_or->evaluate();
    lsh_enable_or->evaluate();
    
    // 3. ALU
    alu->evaluate();
}

void CPU::update()
{
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

