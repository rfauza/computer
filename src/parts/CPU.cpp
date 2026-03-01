#include "CPU.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

CPU::CPU(uint16_t num_bits, const std::string& opcode_string, const std::string& name, uint16_t pc_bits_param) 
    : Part(num_bits, name), low_signal(nullptr)
{
    std::ostringstream oss;
    oss << "CPU 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    // Parse the opcode specification first to determine opcode_bits
    parse_opcodes(opcode_string);
    
    // Determine opcode_bits from actual opcodes (find max opcode value)
    opcode_bits = 0;
    for (const auto& pair : opcode_to_operation)
    {
        uint16_t opcode = pair.first;
        // Count bits needed to represent this opcode
        uint16_t bits_needed = 0;
        uint16_t temp = opcode;
        while (temp > 0)
        {
            bits_needed++;
            temp >>= 1;
        }
        if (bits_needed > opcode_bits)
            opcode_bits = bits_needed;
    }
    
    // Default to num_bits if no opcodes defined or all are zero
    if (opcode_bits == 0)
        opcode_bits = num_bits;
    
    num_decoder_outputs = static_cast<uint16_t>(1u << opcode_bits);
    
    // Create Control Unit with determined opcode_bits and ALU
    // Forward requested PC width (pc_bits_param) to control unit; 0 means default (2*num_bits)
    control_unit = new Control_Unit(num_bits, opcode_bits, pc_bits_param, "control_unit_in_cpu");
    alu = new ALU(num_bits, "alu_in_cpu");

    // Wire decoder outputs to ALU enables (directly)
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

    // Provide a constant low when no opcode exists for an operation
    low_signal = new Signal_Generator("low_signal_in_cpu");
    low_signal->go_low();
    low_signal->evaluate();
    bool* low_out = &low_signal->get_outputs()[0];

    // ALU input layout: [data_a, data_b, enables...]
    uint16_t enable_offset = 2 * num_bits;  // After data_a and data_b

    // Helper lambda to map operation name -> enable index
    auto connect_op = [&](const std::string& opname, uint16_t enable_index)
    {
        bool* src = low_out;
        auto it = operation_to_opcode.find(opname);
        if (it != operation_to_opcode.end())
        {
            uint16_t opcode = it->second;
            if (opcode < num_decoder_outputs)
                src = &decoder_outputs[opcode];
        }
        alu->connect_input(src, enable_offset + enable_index);
    };

    connect_op("ADD", 0);
    connect_op("SUB", 1);
    connect_op("INC", 2);
    connect_op("DEC", 3);
    connect_op("MUL", 4);
    connect_op("AND", 5);
    connect_op("OR", 6);
    connect_op("XOR", 7);
    connect_op("NOT", 8);
    connect_op("RSH", 9);
    connect_op("LSH", 10);
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
    const bool* const* data_a_outputs,
    const bool* const* data_b_outputs,
    const bool* const* data_c_outputs)
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
    
    // Note: Jump address is connected separately via CPU::connect_jump_address_direct
    // or directly via control_unit->connect_jump_address_to_pc
    // These data inputs are for ALU operands only
    
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

bool CPU::get_run_halt_flag() const
{
    return control_unit->get_run_halt_flag();
}

int CPU::get_opcode_for_operation(const std::string& operation_name) const
{
    auto it = operation_to_opcode.find(operation_name);
    if (it != operation_to_opcode.end())
        return static_cast<int>(it->second);
    return -1;
}

bool CPU::wire_halt_opcode(uint16_t halt_opcode_value)
{
    // Wire decoder output for the halt opcode to Control Unit halt signal
    // halt_opcode_value should be the numeric opcode (e.g., 0 for "000" HALT)
    if (halt_opcode_value >= (1u << opcode_bits))
        return false;  // Opcode out of range
    
    bool* decoder_outputs = control_unit->get_decoder_outputs();
    return control_unit->connect_halt_signal(&decoder_outputs[halt_opcode_value]);
}

bool CPU::connect_jump_conditions(const std::vector<std::pair<std::string, uint16_t>>& jump_operation_flag_pairs)
{
    if (jump_operation_flag_pairs.empty())
        return false;
    
    // Convert operation names to opcode indices
    std::vector<std::pair<uint16_t, uint16_t>> jump_conditions;
    
    for (const auto& pair : jump_operation_flag_pairs)
    {
        const std::string& operation_name = pair.first;
        uint16_t flag_index = pair.second;
        
        // Look up opcode for this operation
        auto it = operation_to_opcode.find(operation_name);
        if (it == operation_to_opcode.end())
        {
            std::cerr << "Warning: Jump operation '" << operation_name << "' not found in opcode mapping" << std::endl;
            continue;  // Skip unknown operations
        }
        
        uint16_t opcode = it->second;
        jump_conditions.push_back({opcode, flag_index});
    }
    
    if (jump_conditions.empty())
        return false;
    
    // Wire jump conditions in Control Unit
    return control_unit->connect_jump_instructions(jump_conditions);
}

bool CPU::connect_jump_address(const bool* const* jump_address_outputs, uint16_t num_address_bits)
{
    if (!jump_address_outputs)
        return false;
    
    return control_unit->connect_jump_address_to_pc(jump_address_outputs, num_address_bits);
}

bool* CPU::get_decoder_outputs() const
{
    return control_unit->get_decoder_outputs();
}

bool* CPU::get_cmp_flags() const
{
    return control_unit->get_cmp_flags();
}

bool CPU::wire_flag_write_enable(const bool* signal_ptr)
{
    if (!control_unit)
        return false;
    return control_unit->connect_flag_write_enable(signal_ptr);
}

void CPU::clock_tick()
{
    control_unit->clock_tick();
}

void CPU::evaluate()
{
    // Evaluate in dependency order
    control_unit->evaluate();  // decodes opcode, computes PC, sets write-enable for flags
    alu->evaluate();            // runs comparator to produce fresh flag outputs
    // Evaluate flag register AFTER ALU so it reads fresh comparator outputs
    control_unit->evaluate_flag_register();
}

