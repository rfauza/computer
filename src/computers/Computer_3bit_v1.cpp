#include "Computer_3bit_v1.hpp"
#include <iostream>
#include <sstream>

// Define 3-bit ISA v1 opcodes
const std::string Computer_3bit_v1::ISA_V1_OPCODES =
    "000 HALT\n"
    "001 MOVL\n"
    "010 ADD\n"
    "011 SUB\n"
    "100 CMP\n"
    "101 JEQ\n"
    "110 JGT\n"
    "111 NOP\n";

Computer_3bit_v1::Computer_3bit_v1(const std::string& name)
    : Computer(NUM_BITS, NUM_RAM_ADDR_BITS, PC_BITS, name)
{
    std::ostringstream oss;
    oss << "Computer_3bit_v1 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    // Create CPU with 3-bit ISA v1 and 9-bit PC
    cpu = new CPU(NUM_BITS, ISA_V1_OPCODES, "cpu_3bit_v1", PC_BITS);
    cpu->wire_halt_opcode(0);
    
    // Create Program Memory (9-bit address, 3-bit data) and RAM (6-bit address, 3-bit data)
    program_memory = new Program_Memory(PC_BITS, NUM_BITS, "pm_3bit_v1");
    ram = new Main_Memory(NUM_RAM_ADDR_BITS, NUM_BITS, "ram_3bit_v1");
    
    // Control signals
    pm_write_enable = new Signal_Generator("pm_write_enable");
    pm_write_enable->go_low();
    pm_write_enable->evaluate();
    
    pm_read_enable = new Signal_Generator("pm_read_enable");
    pm_read_enable->go_high();
    pm_read_enable->evaluate();
    
    ram_read_enable = new Signal_Generator("ram_read_enable_a");
    ram_read_enable->go_high();
    ram_read_enable->evaluate();
    
    ram_write_enable = new Signal_Generator("ram_write_enable");
    ram_write_enable->go_high();
    ram_write_enable->evaluate();
    
    // Connect PM opcode outputs to CPU decoder
    const bool** pm_opcode_ptrs = new const bool*[NUM_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        pm_opcode_ptrs[i] = &program_memory->get_outputs()[i];
    }
    bool** pm_address_inputs = new bool*[PC_BITS];
    cpu->connect_program_memory(pm_opcode_ptrs, pm_address_inputs);
    
    for (uint16_t i = 0; i < program_memory->get_decoder_bits(); ++i)
    {
        if (pm_address_inputs[i])
            program_memory->connect_input(pm_address_inputs[i], i);
    }
    
    uint16_t pm_we_index = static_cast<uint16_t>(program_memory->get_decoder_bits() + 4 * program_memory->get_data_bits());
    uint16_t pm_re_index = static_cast<uint16_t>(program_memory->get_decoder_bits() + 4 * program_memory->get_data_bits() + 1);
    program_memory->connect_input(&pm_write_enable->get_outputs()[0], pm_we_index);
    program_memory->connect_input(&pm_read_enable->get_outputs()[0], pm_re_index);
    
    // === RAM address inputs ===
    // PM output layout: [opcode 0-2] [A 3-5] [B 6-8] [C 9-11]
    // Read port 1: [0:A]   Read port 2: [0:B]   Write: [0:C] (high bits gated with MOVL)
    ram_write_addr_high_mux = new AND_Gate*[NUM_BITS];
    bool* cu_decoder = cpu->get_decoder_outputs();
    
    read_addr_high_low = new Signal_Generator("read_addr_high_low");
    read_addr_high_low->go_low();
    read_addr_high_low->evaluate();
    
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        uint16_t pm_a = static_cast<uint16_t>(NUM_BITS + i);      // bits 3-5
        uint16_t pm_b = static_cast<uint16_t>(2 * NUM_BITS + i);  // bits 6-8
        uint16_t pm_c = static_cast<uint16_t>(3 * NUM_BITS + i);  // bits 9-11
        
        // Read port 1 — low bits from A, high bits tied to 0
        ram->connect_input(&program_memory->get_outputs()[pm_a], i);
        ram->connect_input(&read_addr_high_low->get_outputs()[0], static_cast<uint16_t>(NUM_BITS + i));
        
        // Read port 2 — low bits from B, high bits tied to 0
        ram->connect_input(&program_memory->get_outputs()[pm_b], static_cast<uint16_t>(NUM_RAM_ADDR_BITS + i));
        ram->connect_input(&read_addr_high_low->get_outputs()[0], static_cast<uint16_t>(NUM_RAM_ADDR_BITS + NUM_BITS + i));
        
        // Write address low — from C field
        ram->connect_input(&program_memory->get_outputs()[pm_c], static_cast<uint16_t>(2 * NUM_RAM_ADDR_BITS + i));
        
        // Write address high — B field gated with MOVL (so ADD/SUB write to page 0)
        std::ostringstream gate_name;
        gate_name << "ram_write_addr_high_" << i;
        ram_write_addr_high_mux[i] = new AND_Gate(2, gate_name.str());
        if (cu_decoder)
        {
            ram_write_addr_high_mux[i]->connect_input(&cu_decoder[1], 0);  // MOVL enable
            ram_write_addr_high_mux[i]->connect_input(&program_memory->get_outputs()[pm_b], 1);
        }
        ram->connect_input(&ram_write_addr_high_mux[i]->get_outputs()[0],
                           static_cast<uint16_t>(2 * NUM_RAM_ADDR_BITS + NUM_BITS + i));
    }
    
    // === Connect RAM outputs to CPU ALU inputs ===
    data_a_ptrs = new const bool*[NUM_BITS];
    data_b_ptrs = new const bool*[NUM_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        data_a_ptrs[i] = &ram->get_outputs()[i];
        data_b_ptrs[i] = &ram->get_outputs()[NUM_BITS + i];
    }
    
    // PM A field as MOVL literal (data_c)
    data_c_ptrs = new const bool*[NUM_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        data_c_ptrs[i] = &program_memory->get_outputs()[static_cast<uint16_t>(NUM_BITS + i)];
    }
    cpu->connect_data_inputs(data_c_ptrs, data_a_ptrs, data_b_ptrs);
    
    // === Jump address: [A:B:C] ===
    const bool** jump_addr_ptrs = new const bool*[PC_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        jump_addr_ptrs[i]             = &program_memory->get_outputs()[static_cast<uint16_t>(3 * NUM_BITS + i)];  // C
        jump_addr_ptrs[NUM_BITS + i]  = &program_memory->get_outputs()[static_cast<uint16_t>(2 * NUM_BITS + i)];  // B
        jump_addr_ptrs[2 * NUM_BITS + i] = &program_memory->get_outputs()[static_cast<uint16_t>(NUM_BITS + i)];  // A
    }
    cpu->connect_jump_address(jump_addr_ptrs, PC_BITS);
    delete[] jump_addr_ptrs;
    
    // === Jump conditions ===
    std::vector<std::pair<std::string, uint16_t>> jump_conditions;
    jump_conditions.push_back({"JEQ", 0});  // EQ flag
    jump_conditions.push_back({"JGT", 3});  // GT_U flag
    cpu->connect_jump_conditions(jump_conditions);
    
    // === RAM write data mux: MOVL literal vs. CPU result ===
    bool* cpu_result = cpu->get_result_outputs();
    
    ram_data_mux_not = new Inverter(1, "movl_not_in_computer_3bit_v1");
    ram_data_mux_and_literal = new AND_Gate*[NUM_BITS];
    ram_data_mux_and_result  = new AND_Gate*[NUM_BITS];
    ram_data_mux_or = new OR_Gate*[NUM_BITS];
    
    if (cu_decoder)
    {
        ram_data_mux_not->connect_input(&cu_decoder[1], 0);  // !MOVL
        ram_data_mux_not->evaluate();
        
        for (uint16_t i = 0; i < NUM_BITS; ++i)
        {
            std::ostringstream and_lit_name, and_res_name, or_name;
            and_lit_name << "ram_data_mux_and_literal_" << i;
            and_res_name << "ram_data_mux_and_result_" << i;
            or_name      << "ram_data_mux_or_" << i;
            
            ram_data_mux_and_literal[i] = new AND_Gate(2, and_lit_name.str());
            ram_data_mux_and_result[i]  = new AND_Gate(2, and_res_name.str());
            ram_data_mux_or[i]          = new OR_Gate(2, or_name.str());
            
            uint16_t pm_a = static_cast<uint16_t>(NUM_BITS + i);
            ram_data_mux_and_literal[i]->connect_input(&cu_decoder[1], 0);
            ram_data_mux_and_literal[i]->connect_input(&program_memory->get_outputs()[pm_a], 1);
            
            ram_data_mux_and_result[i]->connect_input(&ram_data_mux_not->get_outputs()[0], 0);
            ram_data_mux_and_result[i]->connect_input(&cpu_result[i], 1);
            
            ram_data_mux_or[i]->connect_input(&ram_data_mux_and_literal[i]->get_outputs()[0], 0);
            ram_data_mux_or[i]->connect_input(&ram_data_mux_and_result[i]->get_outputs()[0], 1);
            ram_data_mux_or[i]->evaluate();
            
            ram->connect_input(&ram_data_mux_or[i]->get_outputs()[0],
                               static_cast<uint16_t>(3 * NUM_RAM_ADDR_BITS + i));
        }
    }
    
    // === RAM write enable: MOVL | ADD | SUB ===
    ram_write_or = new OR_Gate(3, "ram_write_or_in_computer_3bit_v1");
    if (cu_decoder && (1u << cpu->get_opcode_bits()) > 2)
    {
        ram_write_or->connect_input(&cu_decoder[1], 0);  // MOVL
        ram_write_or->connect_input(&cu_decoder[2], 1);  // ADD
        ram_write_or->connect_input(&cu_decoder[3], 2);  // SUB
        ram_write_or->evaluate();
    }
    
    // === Two-phase write gating ===
    ram_read_flag = new Signal_Generator("ram_read_flag_in_computer_3bit_v1");
    ram_read_flag->go_high();
    ram_read_flag->evaluate();
    
    ram_read_flag_not = new Inverter(1, "ram_read_flag_not_in_computer_3bit_v1");
    ram_read_flag_not->connect_input(&ram_read_flag->get_outputs()[0], 0);
    ram_read_flag_not->evaluate();
    
    ram_we_gated = new AND_Gate(2, "ram_we_gated_in_computer_3bit_v1");
    ram_we_gated->connect_input(&ram_read_flag_not->get_outputs()[0], 0);
    ram_we_gated->connect_input(&ram_write_or->get_outputs()[0], 1);
    ram_we_gated->evaluate();
    
    ram->connect_input(&ram_we_gated->get_outputs()[0],
                       static_cast<uint16_t>(3 * NUM_RAM_ADDR_BITS + NUM_BITS));
    ram->connect_input(&ram_read_enable->get_outputs()[0],
                       static_cast<uint16_t>(3 * NUM_RAM_ADDR_BITS + NUM_BITS + 1));
    ram->connect_input(&ram_write_enable->get_outputs()[0],
                       static_cast<uint16_t>(3 * NUM_RAM_ADDR_BITS + NUM_BITS + 2));
    
    std::cout << "3-bit Computer v1 initialized with ISA v1" << std::endl;
    std::cout << "  Data width: "    << NUM_BITS           << " bits" << std::endl;
    std::cout << "  RAM addresses: " << num_ram_addresses  << " (6-bit [page:addr])" << std::endl;
    std::cout << "  PM addresses: "  << num_pm_addresses   << std::endl;
    
    delete[] pm_opcode_ptrs;
    delete[] pm_address_inputs;
}

std::string Computer_3bit_v1::get_opcode_name(uint16_t opcode) const
{
    switch (opcode)
    {
        case 0b000: return "HALT";
        case 0b001: return "MOVL";
        case 0b010: return "ADD";
        case 0b011: return "SUB";
        case 0b100: return "CMP";
        case 0b101: return "JEQ";
        case 0b110: return "JGT";
        case 0b111: return "NOP";
        default:    return "UNKNOWN";
    }
}
