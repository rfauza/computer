#include "Computer_3bit_v1.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>

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
    // Set version strings for this computer variant
    computer_version = "3-bit v1";
    ISA_version = "ISA v1";

    // create component name string with ISA suffix
    _create_namestring(name);
    
    // === Instantiate CPU, Program Memory, and RAM ===
    // Create CPU with 3-bit ISA v1 and 9-bit PC
    cpu = new CPU(NUM_BITS, ISA_V1_OPCODES, "cpu_3bit_v1", PC_BITS);
    cpu->wire_halt_opcode(0);
    // Create Program Memory (9-bit address, 3-bit data) and RAM (6-bit address, 3-bit data)
    program_memory = new Program_Memory(PC_BITS, NUM_BITS, "pm_3bit_v1");
    ram = new Main_Memory(NUM_RAM_ADDR_BITS, NUM_BITS, "ram_3bit_v1");
    
    // Connect PM opcode outputs to CPU decoder
    _connect_program_memory_to_CPU_decoder();
    
    // === RAM setup (addressing + write-phase control + data mux) ===
    // PM output layout: [opcode 0-2] [A 3-5] [B 6-8] [C 9-11]
    // Read port 1: [0:A]   Read port 2: CMP→[B:C], others→[0:B]   Write: [0:C] (high bits gated with MOVL)
    _connect_ram_inputs();
    
    // === Connect RAM outputs to CPU ALU inputs ===
    _connect_ram_outputs();
    
    // === Jump address: [A:B:C] ===
    _connect_jump_logic();
    
    // Print constructor success
    _print_architecture_details();
}

void Computer_3bit_v1::_connect_program_memory_to_CPU_decoder()
{
    // Step 1: Extract opcode bits (bits 0-2) from program memory and pass to CPU
    // These bits represent the instruction to execute (HALT, MOVL, ADD, etc.)
    const bool** pm_opcode_ptrs = new const bool*[NUM_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        pm_opcode_ptrs[i] = &program_memory->get_outputs()[i];
    }
    
    // Step 2: Request address input pointers from CPU for program counter
    // The CPU will use these to drive the program memory address lines
    bool** pm_address_inputs = new bool*[PC_BITS];
    cpu->connect_program_memory(pm_opcode_ptrs, pm_address_inputs);

    // Step 3: Connect program memory address inputs to the CPU's program counter outputs
    // This allows the CPU to fetch the next instruction from the correct address
    for (uint16_t i = 0; i < program_memory->get_decoder_bits(); ++i)
    {
        if (pm_address_inputs[i])
            program_memory->connect_input(pm_address_inputs[i], i);
    }

    // Step 4: Connect write and read enable signals to program memory
    // pm_we_index and pm_re_index locate the control signal inputs within the
    // program memory's input array (after address decoder and data ports)
    uint16_t pm_we_index = static_cast<uint16_t>(program_memory->get_decoder_bits() + 4 * program_memory->get_data_bits());
    uint16_t pm_re_index = static_cast<uint16_t>(program_memory->get_decoder_bits() + 4 * program_memory->get_data_bits() + 1);
    program_memory->connect_input(&pm_write_enable->get_outputs()[0], pm_we_index);
    program_memory->connect_input(&pm_read_enable->get_outputs()[0], pm_re_index);

    // Clean up temporary arrays
    delete[] pm_opcode_ptrs;
    delete[] pm_address_inputs;

    // === Create PM opcode decoder ===
    // Decodes the first NUM_BITS outputs of program memory (the opcode field)
    // into a one-hot signal so any opcode can be referenced by index without
    // manual AND/NOT gate trees. Output[4] = CMP (0b100 = 4).
    pm_decoder = new Decoder(NUM_BITS, "pm_opcode_decoder_3bit_v1");
    for (uint16_t i = 0; i < NUM_BITS; ++i)
        pm_decoder->connect_input(&program_memory->get_outputs()[i], i);
}

void Computer_3bit_v1::_connect_ram_inputs()
{
    // Connect RAM Addressing logic to the PM output
    _connect_ram_address_inputs();
    
    // === Implement two-phase write gating to prevent read/write contention ===
    _phase_ram_write_enable();

    // === MULTIPLEX RAM INPUT BETWEEN MOVL LITERAL AND CPU RESULT ===
    _multiplex_RAM_data_inputs();
}

void Computer_3bit_v1::_connect_ram_address_inputs ()
{
    // Create CMP-controlled multiplexers for switching between cmp (2-nibble addr) and ALU (1-nibble addr) addressing.
    _setup_ram_read_muxes();
    // Wire per-bit RAM addresses and write-port controls.
    _wire_ram_address_and_write_controls();
}

void Computer_3bit_v1::_setup_ram_read_muxes()
{
    /* === Connect RAM Addressing input logic to the PM output === */
    // === CMP signal from pm_decoder output[4] (0b100 = 4) ===
    // pm_decoder is already wired to PM opcode bits inside
    // _connect_program_memory_to_CPU_decoder(), so its output[4] goes high
    // whenever the current instruction is CMP, before the CPU evaluates.
    cmp_not = new Inverter(1, "cmp_not_in_computer_3bit_v1");
    cmp_not->connect_input(&pm_decoder->get_outputs()[4], 0);  // NOT(CMP)

    // === Read-port-2 address mux ===
    // ADD/SUB: read port 2 address = [0 : B]   (page 0, addr = B field)
    // CMP:     read port 2 address = [B : C]   (page = B field, addr = C field)
    //
    // Low  bits mux: CMP ? C field : B field
    // High bits mux: CMP ? B field : 0
    ram_read2_addr_mux_low  = new Multiplexer(NUM_BITS, 2, "ram_read2_low_addr_mux");
    ram_read2_addr_mux_high = new Multiplexer(NUM_BITS, 2, "ram_read2_high_addr_mux");
    
    // Low address mux: source[0]=B (!CMP), source[1]=C (CMP)
    const bool* sources_low[2] = {
        &program_memory->get_outputs()[2 * NUM_BITS],  // B field
        &program_memory->get_outputs()[3 * NUM_BITS],  // C field
    };
    const bool* controls_low[2] = {
        &cmp_not->get_outputs()[0],       // !CMP -> select B
        &pm_decoder->get_outputs()[4],    // CMP  -> select C
    };
    ram_read2_addr_mux_low->connect_sources_from_values(sources_low, controls_low);

    // High address mux: source[0]=0 (!CMP), source[1]=B (CMP)
    // read_addr_high_low provides a constant-0 signal; replicate its pointer
    // for each bit since connect_sources() requires per-bit pointer arrays.
    const bool* zeros[NUM_BITS];
    const bool* b_field[NUM_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        zeros[i]   = &read_addr_high_low->get_outputs()[0];
        b_field[i] = &program_memory->get_outputs()[static_cast<uint16_t>(2 * NUM_BITS + i)];
    }
    const bool* const* sources_high[2] = { zeros, b_field };
    const bool* controls_high[2] = {
        &cmp_not->get_outputs()[0],       // !CMP -> select 0
        &pm_decoder->get_outputs()[4],    // CMP  -> select B
    };
    ram_read2_addr_mux_high->connect_sources(sources_high, controls_high);
}

void Computer_3bit_v1::_wire_ram_address_and_write_controls()
{
    // === Configure RAM address inputs for both read ports and write address ===
    // Get references to the CPU's opcode decoder outputs for control logic (MOVL, ADD, SUB write enables)
    cu_decoder = cpu->get_decoder_outputs();
    ram_write_addr_high_mux = new AND_Gate*[NUM_BITS];
    
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        // Extract bit indices for the three operand fields from program memory output
        uint16_t pm_a = static_cast<uint16_t>(NUM_BITS + i);      // bits 3-5  (A field)
        uint16_t pm_b = static_cast<uint16_t>(2 * NUM_BITS + i);  // bits 6-8  (B field)
        uint16_t pm_c = static_cast<uint16_t>(3 * NUM_BITS + i);  // bits 9-11 (C field)

        // === Read port 1: Fetch operand A from RAM ===
        // Low address bits from PM A field; high bits (page) tied to 0.
        ram->connect_input(&program_memory->get_outputs()[pm_a], i);
        ram->connect_input(&read_addr_high_low->get_outputs()[0], static_cast<uint16_t>(NUM_BITS + i));

        // === Read port 2: connect mux outputs ===
        ram->connect_input(&ram_read2_addr_mux_low->get_outputs()[i],
                           static_cast<uint16_t>(NUM_RAM_ADDR_BITS + i));
        ram->connect_input(&ram_read2_addr_mux_high->get_outputs()[i],
                           static_cast<uint16_t>(NUM_RAM_ADDR_BITS + NUM_BITS + i));

        // === Write address low bits: Destination from PM C field ===
        ram->connect_input(&program_memory->get_outputs()[pm_c], static_cast<uint16_t>(2 * NUM_RAM_ADDR_BITS + i));

        // === Write address high bits: Conditional gating on MOVL instruction ===
        // For MOVL: write to page selected by B field.
        // For ADD/SUB: write to page 0 (gate prevents B from affecting address).
        std::ostringstream gate_name;
        gate_name << "ram_write_addr_high_" << i;
        ram_write_addr_high_mux[i] = new AND_Gate(2, gate_name.str());
        ram_write_addr_high_mux[i]->connect_input(&cu_decoder[1], 0);  // MOVL decoder output
        ram_write_addr_high_mux[i]->connect_input(&program_memory->get_outputs()[pm_b], 1);  // B field
        ram->connect_input(&ram_write_addr_high_mux[i]->get_outputs()[0],
                           static_cast<uint16_t>(2 * NUM_RAM_ADDR_BITS + NUM_BITS + i));
    }

    // === Configure RAM write enable from instruction decoder ===
    // OR gate enables write when MOVL, ADD, or SUB instruction is decoded.
    ram_write_or = new OR_Gate(3, "ram_write_or_in_computer_3bit_v1");
    if ((1u << cpu->get_opcode_bits()) > 2)
    {
        ram_write_or->connect_input(&cu_decoder[1], 0);  // MOVL
        ram_write_or->connect_input(&cu_decoder[2], 1);  // ADD
        ram_write_or->connect_input(&cu_decoder[3], 2);  // SUB
        ram_write_or->evaluate();
    }
}

void Computer_3bit_v1::_phase_ram_write_enable()
{
    // === Implement two-phase write gating ===
    // This prevents simultaneous read and write operations to avoid contention
    ram_read_flag = new Signal_Generator("ram_read_flag_in_computer_3bit_v1");
    ram_read_flag->go_high();  // Signal is high to indicate not-reading
    ram_read_flag->evaluate();

    // Invert the read flag to create write enable gate condition
    ram_read_flag_not = new Inverter(1, "ram_read_flag_not_in_computer_3bit_v1");
    ram_read_flag_not->connect_input(&ram_read_flag->get_outputs()[0], 0);
    ram_read_flag_not->evaluate();

    // Combine not-reading signal with instruction-based write enable
    ram_we_gated = new AND_Gate(2, "ram_we_gated_in_computer_3bit_v1");
    ram_we_gated->connect_input(&ram_read_flag_not->get_outputs()[0], 0);  // Not reading
    ram_we_gated->connect_input(&ram_write_or->get_outputs()[0], 1);       // Instruction enables write
    ram_we_gated->evaluate();

    // Connect gated write enable and control signals to RAM
    // These inputs control when and how the RAM operates
    ram->connect_input(&ram_we_gated->get_outputs()[0],
                       static_cast<uint16_t>(3 * NUM_RAM_ADDR_BITS + NUM_BITS));
    ram->connect_input(&ram_read_enable->get_outputs()[0],
                       static_cast<uint16_t>(3 * NUM_RAM_ADDR_BITS + NUM_BITS + 1));
    ram->connect_input(&ram_write_enable->get_outputs()[0],
                       static_cast<uint16_t>(3 * NUM_RAM_ADDR_BITS + NUM_BITS + 2));
}

void Computer_3bit_v1::_multiplex_RAM_data_inputs()
{
    /* Select between ALU result and PM literal for RAM write data */
    
    // Create multiplexer (source 0 for ALU, source 1 for literal)
    int num_sources = 2;
    ram_data_mux = new Multiplexer(NUM_BITS, num_sources, "ram_data_mux_in_computer_3bit_v1");
    
    // get data sources: ALU result from CPU and literal from PM
    const bool* alu_result = cpu->get_result_outputs();
    const bool* pm_literal = &program_memory->get_outputs()[static_cast<uint16_t>(NUM_BITS)];
    
    // create sources array
    const bool* sources[] = { alu_result, pm_literal };
    
    // invert MOVL decoder output to indicate to use CPU result when not MOVL
    Inverter* movl_not = new Inverter(1, "movl_not_for_mux");
    movl_not->connect_input(&cu_decoder[1], 0);
    movl_not->evaluate();
    
    // control signals: MOVL and !MOVL (use literal when MOVL, use ALU result otherwise)
    const bool* controls[] = { &movl_not->get_outputs()[0], &cu_decoder[1] };
    
    // Connect sources and controls to the multiplexer
    ram_data_mux->connect_sources_from_values(sources, controls);
    
    // Connect multiplexed output to RAM input
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        ram->connect_input(&ram_data_mux->get_outputs()[i], static_cast<uint16_t>(3 * NUM_RAM_ADDR_BITS + i));
    }
}

void Computer_3bit_v1::_connect_ram_outputs()
{
    // === Extract data from RAM read ports ===
    // Read port 1 (bits 0-2) via address from A field -> data_a input to CPU
    data_a_ptrs = new const bool*[NUM_BITS];
    // Read port 2 (bits 3-5) via address from B field -> data_b input to CPU
    data_b_ptrs = new const bool*[NUM_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        data_a_ptrs[i] = &ram->get_outputs()[i];
        data_b_ptrs[i] = &ram->get_outputs()[NUM_BITS + i];
    }

    // === Extract literal value for MOVL instruction ===
    // Program memory A field (bits 3-5) provides the constant literal value
    // for MOVL operations -> data_c input to CPU
    data_c_ptrs = new const bool*[NUM_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        data_c_ptrs[i] = &program_memory->get_outputs()[static_cast<uint16_t>(NUM_BITS + i)];
    }

    // === Connect all data sources to CPU ===
    // The CPU uses data_c for MOVL literal, data_a and data_b for ALU operations
    cpu->connect_data_inputs(data_c_ptrs, data_a_ptrs, data_b_ptrs);
}

void Computer_3bit_v1::_connect_jump_logic()
{
    // === Build 9-bit jump address from instruction fields ===
    // Jump target address is formed by concatenating A, B, and C fields:
    // [A(3-5) : B(6-8) : C(9-11)] = [9-bit address]
    // This creates a 9-bit PC target for jump instructions
    const bool** jump_addr_ptrs = new const bool*[PC_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        jump_addr_ptrs[i] = &program_memory->get_outputs()[static_cast<uint16_t>(3 * NUM_BITS + i)];  // C field (bits 0-2)
        jump_addr_ptrs[NUM_BITS + i] = &program_memory->get_outputs()[static_cast<uint16_t>(2 * NUM_BITS + i)];  // B field (bits 3-5)
        jump_addr_ptrs[2 * NUM_BITS + i] = &program_memory->get_outputs()[static_cast<uint16_t>(NUM_BITS + i)];  // A field (bits 6-8)
    }

    // Connect the assembled jump address to the CPU's program counter logic
    cpu->connect_jump_address(jump_addr_ptrs, PC_BITS);
    delete[] jump_addr_ptrs;

    // === Configure conditional jump instructions ===
    // Define which conditions trigger which jump instructions
    std::vector<std::pair<std::string, uint16_t>> jump_conditions;
    // JEQ: Jump if Equal flag is set (result of CMP instruction)
    jump_conditions.push_back({"JEQ", 0});  // EQ flag index from CPU flags
    // JGT: Jump if Greater Than (unsigned) flag is set (result of CMP instruction)
    jump_conditions.push_back({"JGT", 3});  // GT_U (greater than unsigned) flag index
    cpu->connect_jump_conditions(jump_conditions);
    
     // === Wire flag write-enable to CMP opcode output ===
    // Flags should only update when CMP instruction is executed (opcode 4 = 0b100)
    if (cu_decoder && (1u << cpu->get_opcode_bits()) > 4)
    {
        cpu->wire_flag_write_enable(&cu_decoder[4]);  // CMP is opcode 4 (0b100)
    }
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
