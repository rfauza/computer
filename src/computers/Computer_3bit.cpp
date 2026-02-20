#include "Computer_3bit.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <bitset>
#include <filesystem>

// Define 3-bit ISA v1 opcodes
const std::string Computer_3bit::ISA_V1_OPCODES = 
    "000 HALT\n"
    "001 MOVL\n"
    "010 ADD\n"
    "011 SUB\n"
    "100 CMP\n"
    "101 JEQ\n"
    "110 JGT\n"
    "111 NOP\n";

Computer_3bit::Computer_3bit(const std::string& name) : Part(NUM_BITS, name)
{
    std::ostringstream oss;
    oss << "Computer_3bit 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    // Create CPU with 3-bit ISA v1 and request PC width = PC_BITS (9)
    cpu = new CPU(NUM_BITS, ISA_V1_OPCODES, "cpu_3bit", PC_BITS);
    
    // Wire HALT opcode (000 = 0) to Control Unit halt signal
    cpu->wire_halt_opcode(0);
    
    // Create Program Memory: 9-bit address, 3-bit data
    program_memory = new Program_Memory(PC_BITS, NUM_BITS, "pm_3bit");
    
    // Create RAM: 6-bit address, 3-bit data, triple-ported (2R1W)
    ram = new Main_Memory(NUM_RAM_ADDRESS_BITS, NUM_BITS, "ram_3bit");
    
    // Create control signals
    pm_write_enable = new Signal_Generator("pm_write_enable");
    pm_write_enable->go_low();  // PM is read-only during execution
    pm_write_enable->evaluate();
    
    pm_read_enable = new Signal_Generator("pm_read_enable");
    pm_read_enable->go_high();  // PM always readable
    pm_read_enable->evaluate();
    
    ram_read_enable = new Signal_Generator("ram_read_enable_a");
    ram_read_enable->go_high();  // RAM always readable
    ram_read_enable->evaluate();
    
    ram_write_enable = new Signal_Generator("ram_read_enable_b");
    ram_write_enable->go_high();  // RAM read port B always enabled
    ram_write_enable->evaluate();
    
    // Wire components together
    // Connect Program Memory outputs (opcode bits) to CPU decoder inputs
    const bool** pm_opcode_ptrs = new const bool*[NUM_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        pm_opcode_ptrs[i] = &program_memory->get_outputs()[i];
    }

    // Prepare array for PM address inputs; CPU will fill this with PC output pointers
    bool** pm_address_inputs = new bool*[PC_BITS];
    cpu->connect_program_memory(pm_opcode_ptrs, pm_address_inputs);

    // Connect the filled address input pointers into the Program Memory inputs
    // (Program_Memory expects its address inputs to be connected via connect_input)
    for (uint16_t i = 0; i < program_memory->get_decoder_bits(); ++i)
    {
        if (pm_address_inputs[i])
            program_memory->connect_input(pm_address_inputs[i], i);
    }

    // Connect PM control signals (WE/RE)
    uint16_t pm_we_index = static_cast<uint16_t>(program_memory->get_decoder_bits() + 4 * program_memory->get_data_bits());
    uint16_t pm_re_index = static_cast<uint16_t>(program_memory->get_decoder_bits() + 4 * program_memory->get_data_bits() + 1);
    program_memory->connect_input(&pm_write_enable->get_outputs()[0], pm_we_index);
    program_memory->connect_input(&pm_read_enable->get_outputs()[0], pm_re_index);

    // === Connect RAM address inputs ===
    // PM output bits: [opcode 0-2] [C 3-5] [A 6-8] [B 9-11]
    // RAM address A input: PM A field (bits 6-8) - for reading first operand
    // RAM address B input: PM B field (bits 9-11) - for reading second operand
    // RAM address C input: PM C field (bits 3-5) - for writing result
    
    // === Connect RAM address inputs ===
    // PM output bits: [opcode 0-2] [C 3-5] [A 6-8] [B 9-11]
    // Read port 1: address A (only 3 bits used, high bits tied to 0)
    // Read port 2: address B (only 3 bits used, high bits tied to 0)
    // Write port: address = [B:C] = (B << 3) | C (6-bit address for MOVL)
    
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        uint16_t pm_a_field_index = static_cast<uint16_t>(2 * NUM_BITS + i);  // bits 6-8
        uint16_t pm_b_field_index = static_cast<uint16_t>(3 * NUM_BITS + i);  // bits 9-11
        uint16_t pm_c_field_index = static_cast<uint16_t>(NUM_BITS + i);      // bits 3-5
        
        // Read port 1 (address A): bits 0-2 from A field, bits 3-5 tied to 0
        ram->connect_input(&program_memory->get_outputs()[pm_a_field_index], i);
        // Read port 2 (address B): bits 0-2 from B field, bits 3-5 tied to 0
        ram->connect_input(&program_memory->get_outputs()[pm_b_field_index], static_cast<uint16_t>(NUM_RAM_ADDRESS_BITS + i));
        
        // Write port (address C for MOVL): bits 0-2 from C field
        ram->connect_input(&program_memory->get_outputs()[pm_c_field_index], static_cast<uint16_t>(2 * NUM_RAM_ADDRESS_BITS + i));
        // Write port (address C for MOVL): bits 3-5 from B field
        ram->connect_input(&program_memory->get_outputs()[pm_b_field_index], static_cast<uint16_t>(2 * NUM_RAM_ADDRESS_BITS + NUM_BITS + i));
    }
    
    // Read ports only use 3-bit addresses (A and B fields from instruction)
    // High bits (3-5) of read ports remain unconnected (default to 0)
    
    // === Connect RAM data outputs to CPU ALU inputs ===
    // RAM port A and B outputs provide operands for ALU operations
    // These are addressed by the PM A and B fields (already connected to RAM addresses above)
    data_a_ptrs = new const bool*[NUM_BITS];
    data_b_ptrs = new const bool*[NUM_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        data_a_ptrs[i] = &ram->get_outputs()[i];              // Port A output (operand 1)
        data_b_ptrs[i] = &ram->get_outputs()[NUM_BITS + i];   // Port B output (operand 2)
    }
    
    // Connect PM A field (bits 6-8) to CPU data_c input for MOVL immediates
    data_c_ptrs = new const bool*[NUM_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        uint16_t pm_a_field_index = static_cast<uint16_t>(2 * NUM_BITS + i);  // bits 6-8
        data_c_ptrs[i] = &program_memory->get_outputs()[pm_a_field_index];
    }
    
    cpu->connect_data_inputs(data_c_ptrs, data_a_ptrs, data_b_ptrs);

    // === Connect jump address from PM instruction fields ===
    // For jump instructions, use PM C, A, B fields as immediate address [B:A:C]
    // This is separate from the data paths used by ALU operations
    const bool** jump_addr_ptrs = new const bool*[PC_BITS];
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        uint16_t pm_c_field_index = static_cast<uint16_t>(NUM_BITS + i);          // bits 3-5
        uint16_t pm_a_field_index = static_cast<uint16_t>(2 * NUM_BITS + i);      // bits 6-8
        uint16_t pm_b_field_index = static_cast<uint16_t>(3 * NUM_BITS + i);      // bits 9-11
        
        // Arrange as [B:A:C] for address packing
        jump_addr_ptrs[i] = &program_memory->get_outputs()[pm_b_field_index];          // bits 0-2: B
        jump_addr_ptrs[NUM_BITS + i] = &program_memory->get_outputs()[pm_a_field_index];  // bits 3-5: A
        jump_addr_ptrs[2 * NUM_BITS + i] = &program_memory->get_outputs()[pm_c_field_index];  // bits 6-8: C
    }
    cpu->connect_jump_address(jump_addr_ptrs, PC_BITS);
    delete[] jump_addr_ptrs;

    // === Connect jump conditions with comparator flags ===
    // JEQ (opcode 5) ANDed with EQ flag (index 0)
    // JGT (opcode 6) ANDed with GT_U flag (index 3) for unsigned comparison
    std::vector<std::pair<std::string, uint16_t>> jump_conditions;
    jump_conditions.push_back({"JEQ", 0});  // JEQ with EQ flag
    jump_conditions.push_back({"JGT", 3});  // JGT with GT_U flag
    cpu->connect_jump_conditions(jump_conditions);

    // === Create mux for RAM write data input ===
    // MOVL uses PM A field, ADD uses CPU result
    bool* cu_decoder = cpu->get_decoder_outputs();
    bool* cpu_result = cpu->get_result_outputs();
    
    ram_data_mux_not = new Inverter(1, "movl_not_in_computer_3bit");
    ram_data_mux_and_literal = new AND_Gate*[NUM_BITS];
    ram_data_mux_and_result = new AND_Gate*[NUM_BITS];
    ram_data_mux_or = new OR_Gate*[NUM_BITS];
    
    if (cu_decoder)
    {
        // Invert MOVL enable for selecting CPU result
        ram_data_mux_not->connect_input(&cu_decoder[1], 0);  // !MOVL
        ram_data_mux_not->evaluate();
        
        for (uint16_t i = 0; i < NUM_BITS; ++i)
        {
            std::ostringstream and_lit_name, and_res_name, or_name;
            and_lit_name << "ram_data_mux_and_literal_" << i;
            and_res_name << "ram_data_mux_and_result_" << i;
            or_name << "ram_data_mux_or_" << i;
            
            ram_data_mux_and_literal[i] = new AND_Gate(2, and_lit_name.str());
            ram_data_mux_and_result[i] = new AND_Gate(2, and_res_name.str());
            ram_data_mux_or[i] = new OR_Gate(2, or_name.str());
            
            // Mux: select PM A field when MOVL, CPU result otherwise
            uint16_t pm_a_field_index = static_cast<uint16_t>(2 * NUM_BITS + i);
            ram_data_mux_and_literal[i]->connect_input(&cu_decoder[1], 0);  //MOVL enable
            ram_data_mux_and_literal[i]->connect_input(&program_memory->get_outputs()[pm_a_field_index], 1);  // PM A field
            
            ram_data_mux_and_result[i]->connect_input(&ram_data_mux_not->get_outputs()[0], 0);  // !MOVL
            ram_data_mux_and_result[i]->connect_input(&cpu_result[i], 1);  // CPU result
            
            ram_data_mux_or[i]->connect_input(&ram_data_mux_and_literal[i]->get_outputs()[0], 0);
            ram_data_mux_or[i]->connect_input(&ram_data_mux_and_result[i]->get_outputs()[0], 1);
            ram_data_mux_or[i]->evaluate();
            
            // Connect mux output to RAM write data input
            ram->connect_input(&ram_data_mux_or[i]->get_outputs()[0], static_cast<uint16_t>(3 * NUM_RAM_ADDRESS_BITS + i));
        }
    }

    // === Wire MOVL, ADD and SUB opcodes to RAM write enable ===
    ram_write_or = new OR_Gate(3, "ram_write_or_in_computer_3bit");
    unsigned int opcode_count = 1u << cpu->get_opcode_bits();
    if (cu_decoder && opcode_count > 2)
    {
        ram_write_or->connect_input(&cu_decoder[1], 0);  // MOVL
        ram_write_or->connect_input(&cu_decoder[2], 1);  // ADD
        ram_write_or->connect_input(&cu_decoder[3], 2);  // SUB
        ram_write_or->evaluate();
    }
    
    // === Create RAM write gating: control WE based on read/write phase ===
    // ram_we_gated = NOT(ram_read_flag) AND ram_write_or
    // During reads (ram_read_flag=1): WE=0, no writes
    // During write setup (ram_read_flag=0): WE=ram_write_or
    ram_read_flag = new Signal_Generator("ram_read_flag_in_computer_3bit");
    ram_read_flag->go_high();  // Start in read mode
    ram_read_flag->evaluate();
    
    ram_read_flag_not = new Inverter(1, "ram_read_flag_not_in_computer_3bit");
    ram_read_flag_not->connect_input(&ram_read_flag->get_outputs()[0], 0);
    ram_read_flag_not->evaluate();
    
    ram_we_gated = new AND_Gate(2, "ram_we_gated_in_computer_3bit");
    ram_we_gated->connect_input(&ram_read_flag_not->get_outputs()[0], 0);  // NOT ram_read_flag
    ram_we_gated->connect_input(&ram_write_or->get_outputs()[0], 1);       // ram_write_or
    ram_we_gated->evaluate();
    
    // Connect gated write enable to RAM (instead of raw ram_write_or)
    ram->connect_input(&ram_we_gated->get_outputs()[0], static_cast<uint16_t>(3 * NUM_RAM_ADDRESS_BITS + NUM_BITS));

    // RAM read enables always high
    ram->connect_input(&ram_read_enable->get_outputs()[0], static_cast<uint16_t>(3 * NUM_RAM_ADDRESS_BITS + NUM_BITS + 1));  // RE_A
    ram->connect_input(&ram_write_enable->get_outputs()[0], static_cast<uint16_t>(3 * NUM_RAM_ADDRESS_BITS + NUM_BITS + 2));  // RE_B

    std::cout << "3-bit Computer initialized with ISA v1" << std::endl;
    std::cout << "  Data width: " << NUM_BITS << " bits" << std::endl;
    std::cout << "  RAM addresses: " << NUM_RAM_ADDRESSES << " (6-bit MOVL addressing: [B:C])" << std::endl;
    std::cout << "  PM addresses: " << NUM_PM_ADDRESSES << std::endl;

    // Initialize member vector signal generators for PM loading (allocate dynamically)
    pm_load_addr_sigs = new std::vector<Signal_Generator>();
    pm_load_addr_sigs->resize(PC_BITS);
    
    pm_load_data_sigs = new std::vector<Signal_Generator>();
    pm_load_data_sigs->resize(4 * NUM_BITS);
    
    // Initialize RAM address signal generators (not used anymore but kept for compatibility)
    ram_addr_sigs = new std::vector<Signal_Generator>();
    ram_addr_sigs->resize(NUM_BITS);

    // Clean up temporary arrays (CPU/Program_Memory retain pointers)
    delete[] pm_opcode_ptrs;
    delete[] pm_address_inputs;
    // Don't delete data_a/b/c_ptrs - they're member variables that persist
}

Computer_3bit::~Computer_3bit()
{
    delete cpu;
    delete program_memory;
    delete ram;
    delete pm_write_enable;
    delete pm_read_enable;
    delete ram_write_enable;
    delete ram_read_enable;
    delete ram_write_or;
    delete ram_read_flag;
    delete ram_read_flag_not;
    delete ram_we_gated;
    delete ram_data_mux_not;
    
    // Delete pointer arrays
    delete[] data_a_ptrs;
    delete[] data_b_ptrs;
    delete[] data_c_ptrs;
    
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        delete ram_data_mux_and_literal[i];
        delete ram_data_mux_and_result[i];
        delete ram_data_mux_or[i];
    }
    delete[] ram_data_mux_and_literal;
    delete[] ram_data_mux_and_result;
    delete[] ram_data_mux_or;
    
    // Delete dynamically allocated vectors
    delete pm_load_addr_sigs;
    delete pm_load_data_sigs;
    delete ram_addr_sigs;
}

bool Computer_3bit::load_program(const std::string& filename)
{
    std::ifstream file(filename);
    std::string resolved_path = filename;
    if (!file.is_open())
    {
        // Try common fallback locations relative to build/executable directory
        std::vector<std::string> candidates = {
            std::string("../") + filename,
            std::string("../src/") + filename,
            std::string("./") + filename,
            std::string("")
        };

        for (const auto& cand : candidates)
        {
            if (cand.empty()) continue;
            if (std::filesystem::exists(cand))
            {
                file.open(cand);
                if (file.is_open())
                {
                    resolved_path = cand;
                    break;
                }
            }
        }

        if (!file.is_open())
        {
            std::cerr << "Error: Could not open program file: " << filename << std::endl;
            return false;
        }
        else
        {
            std::cout << "Opened program file: " << resolved_path << std::endl;
        }
    }
    
    std::cout << "Loading program from: " << filename << std::endl;
    
    uint16_t address = 0;
    std::string line;
    
    // Use member vector signal generators for loading (keep alive after function returns)
    uint16_t decoder_bits = program_memory->get_decoder_bits();
    uint16_t data_bits = program_memory->get_data_bits();
    uint16_t data_inputs_start = decoder_bits; // index where opcode/C/A/B inputs start

    while (std::getline(file, line) && address < NUM_PM_ADDRESSES)
    {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line.find_first_not_of(" \t\r\n") == std::string::npos)
            continue;

        std::istringstream iss(line);
        std::string opcode_str, c_str, a_str, b_str;

        // Parse line: opcode C A B
        if (!(iss >> opcode_str >> c_str >> a_str >> b_str))
        {
            std::cerr << "Warning: Malformed line at address " << address << ": " << line << std::endl;
            continue;
        }

        // Convert to integers
        uint16_t opcode = from_binary(opcode_str);
        uint16_t c_val = from_binary(c_str);
        uint16_t a_val = from_binary(a_str);
        uint16_t b_val = from_binary(b_str);

        // Validate bit widths
        if (opcode >= (1u << NUM_BITS) || c_val >= (1u << NUM_BITS) || 
            a_val >= (1u << NUM_BITS) || b_val >= (1u << NUM_BITS))
        {
            std::cerr << "Error: Value out of range at address " << address << std::endl;
            continue;
        }

        std::cout << "  [" << std::setw(3) << std::setfill('0') << address << "] "
              << to_binary(opcode, NUM_BITS) << " "
              << to_binary(c_val, NUM_BITS) << " "
              << to_binary(a_val, NUM_BITS) << " "
              << to_binary(b_val, NUM_BITS) << " ; "
              << get_opcode_name(opcode) << " "
              << c_val << " " << a_val << " " << b_val << std::endl;

        // Drive address inputs using member signal generators (temporary)
        for (uint16_t i = 0; i < decoder_bits; ++i)
        {
            bool bit = ((address >> i) & 1) != 0;
            if (bit) (*pm_load_addr_sigs)[i].go_high(); else (*pm_load_addr_sigs)[i].go_low();
            (*pm_load_addr_sigs)[i].evaluate();
            program_memory->connect_input(&(*pm_load_addr_sigs)[i].get_outputs()[0], i);
        }

        // Drive data inputs (opcode, C, A, B) using member signal generators
        uint16_t values[4] = { opcode, c_val, a_val, b_val };
        for (uint16_t reg = 0; reg < 4; ++reg)
        {
            for (uint16_t b = 0; b < data_bits; ++b)
            {
                uint16_t idx = reg * data_bits + b;
                bool bit = ((values[reg] >> b) & 1) != 0;
                if (bit) (*pm_load_data_sigs)[idx].go_high(); else (*pm_load_data_sigs)[idx].go_low();
                (*pm_load_data_sigs)[idx].evaluate();
                program_memory->connect_input(&(*pm_load_data_sigs)[idx].get_outputs()[0], data_inputs_start + idx);
            }
        }

        // Pulse write enable to write this instruction into memory
        pm_write_enable->go_high();
        pm_write_enable->evaluate();
        program_memory->evaluate();
        program_memory->update();
        pm_write_enable->go_low();
        pm_write_enable->evaluate();

        address++;
    }
    // After loading, reconnect PM address inputs back to PC outputs so CPU can drive them
    bool* pc_outputs = cpu->get_pc_outputs();
    // Connect ALL PC bits to PM address inputs (9 bits for 512 addresses)
    for (uint16_t i = 0; i < PC_BITS; ++i)
    {
        program_memory->connect_input(&pc_outputs[i], i);
    }
    
    // Evaluate PM with PC now driving the address (should read instruction at PC=0)
    program_memory->evaluate();

    // After loading, disconnect load data signals from PM (they stay at 0)
    // PM will only be read during execution via the AND/OR decoder logic
    for (auto& sig : *pm_load_data_sigs)
    {
        sig.go_low();
        sig.evaluate();
    }
    
    std::cout << "Loaded " << address << " instructions" << std::endl;
    file.close();
    return true;
}

void Computer_3bit::run_interactive()
{
    std::cout << "\n=== Starting Interactive Execution ===\n" << std::endl;
    std::cout << "Press Enter to execute each instruction..." << std::endl;
    
    reset();
    
    // Initial evaluation to sync PM with PC=0
    program_memory->evaluate();
    
    bool running = true;
    while (running)
    {
        print_state();
        
        // Wait for user input
        std::cout << "\nPress Enter to continue (or 'q' to quit): ";
        std::string input;
        std::getline(std::cin, input);
        
        if (input == "q" || input == "Q")
        {
            std::cout << "Execution stopped by user." << std::endl;
            break;
        }
        
        // Execute one clock cycle
        running = clock_tick();
        
        // After update() changes PC, re-evaluate PM to read from new address
        program_memory->evaluate();
        
        if (!running)
        {
            std::cout << "\n=== Program HALTED ===" << std::endl;
            break;  // Exit loop immediately after halting
        }
    }
}

bool Computer_3bit::clock_tick()
{
    // Two-phase clock cycle:
    
    if (is_running)
    {
        // Phase 1: Evaluate (combinational logic computes next values)
        evaluate();
    }
    else
    {
        std::cout << "Computer is halted. No further execution." << std::endl;
        return false;  // Remain halted
    }
    
    // Debug: print RAM inputs after combinational evaluation to trace signals
    // std::cout << "[DBG][Computer_3bit] RAM input signals:" << std::endl;
    // ram->print_inputs();
    
    // Phase 2: Update (storage elements latch new values)
    // Update all storage elements together: PC, RAM, registers
    // cpu->update();
    // ram->update();
    
    // Check if halted (run/halt flag from control unit)
    is_running = cpu->get_run_halt_flag();
    
    return is_running;
}

void Computer_3bit::print_state() const
{
    std::cout << "\n" << std::string(50, '=') << std::endl;
    
    // Print PC
    bool* pc_outputs = cpu->get_pc_outputs();
    uint16_t pc_value = 0;
    for (uint16_t i = 0; i < PC_BITS; ++i)
    {
        pc_value |= (pc_outputs[i] ? 1 : 0) << i;
    }
    std::cout << "PC: " << std::setw(3) << std::setfill('0') << pc_value 
              << " (" << to_binary(pc_value, PC_BITS) << ")" << std::endl;
    
    // Print current instruction from PM
    bool* pm_outputs = program_memory->get_outputs();
    uint16_t opcode = 0, c_val = 0, a_val = 0, b_val = 0;
    
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        opcode |= (pm_outputs[i] ? 1 : 0) << i;
        c_val |= (pm_outputs[NUM_BITS + i] ? 1 : 0) << i;
        a_val |= (pm_outputs[2 * NUM_BITS + i] ? 1 : 0) << i;
        b_val |= (pm_outputs[3 * NUM_BITS + i] ? 1 : 0) << i;
    }
    
    std::cout << "Instruction: " << to_binary(opcode, NUM_BITS) << " "
              << to_binary(c_val, NUM_BITS) << " "
              << to_binary(a_val, NUM_BITS) << " "
              << to_binary(b_val, NUM_BITS) << " ; "
              << get_opcode_name(opcode) << " "
              << c_val << " " << a_val << " " << b_val << std::endl;
    
    // Print RAM contents (8 pages of 8)
    ram->print_pages(8);
    
    std::cout << std::string(50, '=') << std::endl;
}

void Computer_3bit::reset()
{
    // Reset PC to 0
    // TODO: Implement PC reset through control unit
    
    // Clear RAM
    // TODO: Write zeros to all RAM addresses
    
    std::cout << "Computer reset to initial state" << std::endl;
}

void Computer_3bit::toggle_ram_read_flag(bool flag_high)
{
    if (flag_high)
    {
        ram_read_flag->go_high();
    }
    else
    {
        ram_read_flag->go_low();
    }
    ram_read_flag->evaluate();
    ram_read_flag_not->evaluate();
    ram_we_gated->evaluate();
}

void Computer_3bit::evaluate()
{
    // Evaluation order for single-cycle 2R1W operation:
    // 1. PM provides instruction (opcode + addresses)
    // 2. RAM reads (combinational - addresses to data out)
    // 3. CPU computes (uses RAM read data, produces result)
    // 4. RAM write control (mux selects write data, WE goes high)
    //    Then update() will latch the write
    
    program_memory->evaluate();
    // std::cout << "ram contents at beginning of loop: ";
    // ram->print_inputs();  // Debug: print RAM inputs before evaluation
    
    // Phase 1: Set read flag HIGH (prevents writes during reads)
    toggle_ram_read_flag(true);
    
    ram->evaluate();  // RAM reads with new addresses (WE gated to 0)
    cpu->evaluate();  // CPU computes using RAM outputs

    // Print ALU comparator flags for debugging (EQ, NEQ, LT_U, GT_U, LT_S, GT_S)
    bool* alu_out = cpu->get_result_outputs();
    if (alu_out)
    {
        std::cout << "[DBG][Compare Flags] ";
        std::cout << "EQ=" << (alu_out[NUM_BITS + 0] ? 1 : 0) << " ";
        std::cout << "NEQ=" << (alu_out[NUM_BITS + 1] ? 1 : 0) << " ";
        std::cout << "LT_U=" << (alu_out[NUM_BITS + 2] ? 1 : 0) << " ";
        std::cout << "GT_U=" << (alu_out[NUM_BITS + 3] ? 1 : 0) << " ";
        std::cout << "LT_S=" << (alu_out[NUM_BITS + 4] ? 1 : 0) << " ";
        std::cout << "GT_S=" << (alu_out[NUM_BITS + 5] ? 1 : 0) << std::endl;
    }
    
    // Evaluate RAM write control logic
    ram_data_mux_not->evaluate();
    for (uint16_t i = 0; i < NUM_BITS; ++i)
    {
        ram_data_mux_and_literal[i]->evaluate();
        ram_data_mux_and_result[i]->evaluate();
        ram_data_mux_or[i]->evaluate();
    }
    // ram_write_or->evaluate();
    // ram_write_or->print_io();  // Debug: print RAM write enable OR gate IO after evaluation
    // DEBUG: if write enable is active, log relevant signals to trace unexpected writes
    // if (ram_write_or->get_outputs()[0])
    // {
    //     // Gather decoder bits and write address/data
    //     bool* dec_ptr = cpu->get_decoder_outputs();
    //     uint16_t dec0 = dec_ptr ? (dec_ptr[0] ? 1 : 0) : 0;
    //     uint16_t dec1 = dec_ptr ? (dec_ptr[1] ? 1 : 0) : 0;
    //     uint16_t dec2 = dec_ptr ? (dec_ptr[2] ? 1 : 0) : 0;
    //     uint16_t write_addr = 0;
    //     for (uint16_t i = 0; i < NUM_BITS; ++i)
    //     {
    //         write_addr |= (program_memory->get_outputs()[NUM_BITS + i] ? 1 : 0) << i; // C field
    //     }
    //     uint16_t write_data = 0;
    //     for (uint16_t i = 0; i < NUM_BITS; ++i)
    //     {
    //         write_data |= (ram_data_mux_or[i]->get_outputs()[0] ? 1 : 0) << i;
    //     }
    //     std::cout << "[DBG][Computer_3bit] decoder=" << dec0 << dec1 << dec2
    //               << " ram_we=1 addr=" << write_addr << " data=" << write_data << std::endl;
    // }
    // Re-evaluate RAM so write_selects and registers see the new WE/data
    // This ensures the write_select AND gates pick up the just-computed write enable
    // before update() latches the register values.
    
    // Phase 2: Set read flag LOW (allow writes during write setup)
    ram_write_or->evaluate();
    toggle_ram_read_flag(false);
    
    ram->evaluate();  // RAM evaluates with WE now controlled by ram_write_or
}

void Computer_3bit::update()
{
    // Called when this component is a downstream of another
    // We don't need to do anything special here since clock_tick() handles the full cycle
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

std::string Computer_3bit::to_binary(uint16_t value, uint16_t bits) const
{
    std::string result;
    for (int i = bits - 1; i >= 0; --i)
    {
        result += ((value >> i) & 1) ? '1' : '0';
    }
    return result;
}

uint16_t Computer_3bit::from_binary(const std::string& binary) const
{
    // Try parsing as binary first
    if (binary.find_first_not_of("01") == std::string::npos)
    {
        uint16_t value = 0;
        for (char c : binary)
        {
            value = (value << 1) | (c == '1' ? 1 : 0);
        }
        return value;
    }
    
    // Otherwise parse as decimal
    return static_cast<uint16_t>(std::stoi(binary));
}

std::string Computer_3bit::get_opcode_name(uint16_t opcode) const
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
        default: return "UNKNOWN";
    }
}
