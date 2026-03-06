#include "Computer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>

Computer::Computer(uint16_t num_bits_, uint16_t num_ram_addr_bits_,
                   uint16_t pc_bits_, const std::string& name)
    : Part(num_bits_, name),
      num_bits(num_bits_),
      num_ram_address_bits(num_ram_addr_bits_),
      num_ram_addresses(static_cast<uint16_t>(1u << num_ram_addr_bits_)),
      pc_bits(pc_bits_),
      num_pm_addresses(static_cast<uint16_t>(1u << pc_bits_)),
      cpu(nullptr),
      program_memory(nullptr),
      ram(nullptr),
      rampage(nullptr),
      opcodepage(nullptr),
      pm_write_enable(nullptr),
      pm_read_enable(nullptr),
      ram_write_enable(nullptr),
      ram_read_enable(nullptr),
      read_addr_high_low(nullptr),
      ram_write_or(nullptr),
      ram_read_flag(nullptr),
      ram_read_flag_not(nullptr),
      ram_we_gated(nullptr),
      ram_data_mux(nullptr),
      ram_write_addr_high_mux(nullptr),
      pm_load_addr_sigs(nullptr),
      pm_load_data_sigs(nullptr),
      pm_zero_sigs(nullptr),
      ram_addr_sigs(nullptr),
      data_a_ptrs(nullptr),
      data_b_ptrs(nullptr),
      data_c_ptrs(nullptr),
      is_running(true),
      execution_count(0),
      computer_version(""),
      ISA_version(""),
      pm_decoder(nullptr),
      cmp_not(nullptr),
      ram_read2_addr_mux_low(nullptr),
      ram_read2_addr_mux_high(nullptr)
{
    // Signal generators for PM loading are sized here since pc_bits / num_bits
    // are already known. Subclass constructors do not need to create them.
    pm_load_addr_sigs = new std::vector<Signal_Generator>();
    pm_load_addr_sigs->resize(pc_bits);
    
    pm_load_data_sigs = new std::vector<Signal_Generator>();
    pm_load_data_sigs->resize(4 * num_bits);
    
    pm_zero_sigs = new std::vector<Signal_Generator>();
    pm_zero_sigs->resize(4 * num_bits);
    for (auto& sig : *pm_zero_sigs)
    {
        sig.go_low();
        sig.evaluate();
    }
    
    ram_addr_sigs = new std::vector<Signal_Generator>();
    ram_addr_sigs->resize(num_bits);
    
    // Create control signals for PM and RAM
    // PM W/E used for loading program
    pm_write_enable = new Signal_Generator("pm_write_enable");
    pm_write_enable->go_low();
    pm_write_enable->evaluate();
    
    // PM should be always readable
    pm_read_enable = new Signal_Generator("pm_read_enable");
    pm_read_enable->go_high();
    pm_read_enable->evaluate();
    
    ram_read_enable = new Signal_Generator("ram_read_enable");
    ram_read_enable->go_high();
    ram_read_enable->evaluate();
    
    ram_write_enable = new Signal_Generator("ram_write_enable");
    ram_write_enable->go_high();
    ram_write_enable->evaluate();
    
    // Constant low signal for tying read address high bits to 0
    read_addr_high_low = new Signal_Generator("read_addr_high_low");
    read_addr_high_low->go_low();
    read_addr_high_low->evaluate();

    // Create page registers (initially all 0's)
    rampage = new Register(num_bits, "rampage");
    opcodepage = new Register(num_bits, "opcodepage");
}

Computer::~Computer()
{
    delete cpu;
    delete program_memory;
    delete ram;
    delete rampage;
    delete opcodepage;
    delete pm_write_enable;
    delete pm_read_enable;
    delete ram_write_enable;
    delete ram_read_enable;
    delete read_addr_high_low;
    delete ram_write_or;
    delete ram_read_flag;
    delete ram_read_flag_not;
    delete ram_we_gated;
    delete ram_data_mux;
    
    delete[] data_a_ptrs;
    delete[] data_b_ptrs;
    delete[] data_c_ptrs;
    
    if (ram_write_addr_high_mux)
    {
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            delete ram_write_addr_high_mux[i];
        }
    }
    delete[] ram_write_addr_high_mux;
    
    delete pm_load_addr_sigs;
    delete pm_load_data_sigs;
    delete pm_zero_sigs;
    delete ram_addr_sigs;

    // Delete PM opcode decoder and read-port-2 address muxes (null-safe)
    delete pm_decoder;
    delete cmp_not;
    delete ram_read2_addr_mux_low;
    delete ram_read2_addr_mux_high;
}

bool Computer::load_program(const std::string& filename)
{
    std::ifstream file(filename);
    std::string resolved_path = filename;
    if (!file.is_open())
    {
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
    
    uint16_t decoder_bits      = program_memory->get_decoder_bits();
    uint16_t data_bits         = program_memory->get_data_bits();
    uint16_t data_inputs_start = decoder_bits;

    while (std::getline(file, line) && address < num_pm_addresses)
    {
        if (line.empty() || line[0] == '#' ||
            line.find_first_not_of(" \t\r\n") == std::string::npos)
            continue;

        std::istringstream iss(line);
        std::string opcode_str, a_str, b_str, c_str;
        if (!(iss >> opcode_str >> a_str >> b_str >> c_str))
        {
            std::cerr << "Warning: Malformed line at address " << address
                      << ": " << line << std::endl;
            continue;
        }

        uint16_t opcode = from_binary(opcode_str);
        uint16_t a_val  = from_binary(a_str);
        uint16_t b_val  = from_binary(b_str);
        uint16_t c_val  = from_binary(c_str);

        if (opcode >= (1u << num_bits) || a_val >= (1u << num_bits) ||
            b_val  >= (1u << num_bits) || c_val >= (1u << num_bits))
        {
            std::cerr << "Error: Value out of range at address " << address << std::endl;
            continue;
        }

        std::cout << "  [" << std::setw(3) << std::setfill('0') << address << "] "
                  << to_binary(opcode, num_bits) << " "
                  << to_binary(a_val,  num_bits) << " "
                  << to_binary(b_val,  num_bits) << " "
                  << to_binary(c_val,  num_bits) << " ; "
                  << get_opcode_name(opcode) << " "
                  << a_val << " " << b_val << " " << c_val << std::endl;

        for (uint16_t i = 0; i < decoder_bits; ++i)
        {
            bool bit = ((address >> i) & 1) != 0;
            if (bit) (*pm_load_addr_sigs)[i].go_high();
            else     (*pm_load_addr_sigs)[i].go_low();
            (*pm_load_addr_sigs)[i].evaluate();
            program_memory->connect_input(&(*pm_load_addr_sigs)[i].get_outputs()[0], i);
        }

        uint16_t values[4] = { opcode, a_val, b_val, c_val };
        for (uint16_t reg = 0; reg < 4; ++reg)
        {
            for (uint16_t b = 0; b < data_bits; ++b)
            {
                uint16_t idx = reg * data_bits + b;
                bool bit = ((values[reg] >> b) & 1) != 0;
                if (bit) (*pm_load_data_sigs)[idx].go_high();
                else     (*pm_load_data_sigs)[idx].go_low();
                (*pm_load_data_sigs)[idx].evaluate();
                program_memory->connect_input(
                    &(*pm_load_data_sigs)[idx].get_outputs()[0],
                    static_cast<uint16_t>(data_inputs_start + idx));
            }
        }

        pm_write_enable->go_high();
        pm_write_enable->evaluate();
        program_memory->evaluate();
        pm_write_enable->go_low();
        pm_write_enable->evaluate();

        address++;
    }

    bool* pc_outputs = cpu->get_pc_outputs();
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        program_memory->connect_input(&pc_outputs[i], i);
    }
    program_memory->evaluate();

    // Reconnect PM data inputs to permanent zeros to prevent stale write data
    // from corrupting subsequent memory operations
    for (uint16_t b = 0; b < 4 * data_bits; ++b)
    {
        program_memory->connect_input(&(*pm_zero_sigs)[b].get_outputs()[0],
                                      static_cast<uint16_t>(data_inputs_start + b));
    }
    
    std::cout << "Loaded " << address << " instructions" << std::endl;
    file.close();
    return true;
}

void Computer::run(bool interactive)
{
    std::cout << "\n=== Starting Execution";

    program_memory->evaluate();

    while (is_running)
    {
        if (interactive) // Prompt user to press Enter once per cycle
        {
            std::cout << "\nPress Enter to continue (or 'q' to quit): ";
            std::string input;
            std::getline(std::cin, input);
            if (input == "q" || input == "Q")
            {
                std::cout << "Execution stopped by user." << std::endl;
                break;
            }
        }

        clock_tick();
        print_state();
    }

    std::cout << "\n=== Program HALTED ===" << std::endl;
    // print_state();
}

bool Computer::clock_tick()
{
    if (is_running)
    {
        evaluate();
    }
    else
    {
        std::cout << "Computer is halted. No further execution." << std::endl;
        return false;
    }
    
    execution_count++;
    is_running = cpu->get_run_halt_flag();
    return is_running;
}

void Computer::print_state() const
{
    std::cout << "\n" << std::string(50, '=') << std::endl;
    
    uint16_t pc_value = program_memory->get_selected_address();
    std::cout << "PC: " << std::setw(3) << std::setfill('0') << pc_value
              << " (" << to_binary(pc_value, pc_bits) << ")"
              << "    Execution Count: " << execution_count << std::endl;
    
    bool* pm_outputs = program_memory->get_outputs();
    uint16_t opcode = 0, a_val = 0, b_val = 0, c_val = 0;
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        opcode |= (pm_outputs[i]               ? 1 : 0) << i;
        a_val  |= (pm_outputs[num_bits + i]     ? 1 : 0) << i;
        b_val  |= (pm_outputs[2 * num_bits + i] ? 1 : 0) << i;
        c_val  |= (pm_outputs[3 * num_bits + i] ? 1 : 0) << i;
    }
    
    std::cout << "Instruction: "
              << to_binary(opcode, num_bits) << " "
              << to_binary(a_val,  num_bits) << " "
              << to_binary(b_val,  num_bits) << " "
              << to_binary(c_val,  num_bits) << " ; "
              << get_opcode_name(opcode) << " "
              << a_val << " " << b_val << " " << c_val << std::endl;
              
    // Print comparator flags from CPU's stored flag register (labelled)
    if (cpu)
    {
        bool* stored_flags = cpu->get_cmp_flags();
        if (stored_flags)
        {
            std::cout << "Compare Flags: "
                      << "EQ="  << (stored_flags[0] ? '1' : '0') << ", "
                      << "NEQ=" << (stored_flags[1] ? '1' : '0') << ", "
                      << "LT_U="<< (stored_flags[2] ? '1' : '0') << ", "
                      << "GT_U="<< (stored_flags[3] ? '1' : '0') << ", "
                      << "LT_S="<< (stored_flags[4] ? '1' : '0') << ", "
                      << "GT_S="<< (stored_flags[5] ? '1' : '0') << std::endl;
        }
    }
    
    ram->print_pages(8);
    ram->print_io();
    
    std::cout << std::string(50, '=') << std::endl;
}

void Computer::reset()
{
    // TODO: Reset PC through control unit
    // TODO: Write zeros to all RAM addresses
    std::cout << "Computer reset to initial state" << std::endl;
}

void Computer::reset_pc()
{
    cpu->set_run_halt_flag(true);
    cpu->reset_pc();
    is_running = true;
    sync_pc();
}

void Computer::reset_ram()
{
    ram->zero_all();
}

void Computer::reset_all()
{
    // Zero all RAM
    ram->zero_all();

    // Zero all PM instructions
    for (uint16_t addr = 0; addr < num_pm_addresses; ++addr)
    {
        write_pm_instruction(addr, 0, 0, 0, 0);
    }

    // Reset PC to 0 and clear halt
    cpu->set_run_halt_flag(true);
    cpu->reset_pc();
    is_running = true;
    sync_pc();
}

void Computer::clear_halt()
{
    cpu->set_run_halt_flag(true);
    is_running = true;
}

void Computer::toggle_ram_read_flag(bool flag_high)
{
    if (flag_high) ram_read_flag->go_high();
    else           ram_read_flag->go_low();
    ram_read_flag->evaluate();
    ram_read_flag_not->evaluate();
    ram_we_gated->evaluate();
}

void Computer::evaluate()
{
    program_memory->evaluate();

    // Evaluate PM opcode decoder (if wired by subclass).
    // Must run immediately after program_memory so all downstream opcode-based
    // gates see stable one-hot outputs before RAM reads.
    if (pm_decoder)
        pm_decoder->evaluate();

    // Evaluate read-port-2 address mux (if wired by subclass).
    // Must run before ram->evaluate() so RAM sees the correct address.
    if (cmp_not)
        cmp_not->evaluate();
    if (ram_read2_addr_mux_low)
        ram_read2_addr_mux_low->evaluate();
    if (ram_read2_addr_mux_high)
        ram_read2_addr_mux_high->evaluate();

    // Phase 1: read flag HIGH → WE gated to 0, safe to read RAM
    toggle_ram_read_flag(true);

    ram->evaluate();   // combinational reads
    cpu->evaluate();   // compute next values
    
    // Evaluate write-path logic
    // New design: a single `Multiplexer` device provides the write-data outputs.
    if (ram_data_mux)
    {
        ram_data_mux->evaluate();
    }

    // Allow subclass to evaluate ISA-specific gates before write-address gates use them
    evaluate_isa_write_gates();

    // Evaluate any per-bit write-address high-bit gates if allocated by subclass
    if (ram_write_addr_high_mux)
    {
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            if (ram_write_addr_high_mux[i])
                ram_write_addr_high_mux[i]->evaluate();
        }
    }
    
    // Phase 2: read flag LOW → WE follows ram_write_or
    ram_write_or->evaluate();
    toggle_ram_read_flag(false);
    
    ram->evaluate();   // latch writes
}

std::string Computer::to_binary(uint16_t value, uint16_t bits) const
{
    std::string result;
    for (int i = bits - 1; i >= 0; --i)
    {
        result += ((value >> i) & 1) ? '1' : '0';
    }
    return result;
}

uint16_t Computer::from_binary(const std::string& binary) const
{
    if (binary.find_first_not_of("01") == std::string::npos)
    {
        uint16_t value = 0;
        for (char c : binary)
        {
            value = static_cast<uint16_t>((value << 1) | (c == '1' ? 1 : 0));
        }
        return value;
    }
    return static_cast<uint16_t>(std::stoi(binary));
}

void Computer::evaluate_isa_write_gates()
{
    // Default no-op: subclasses may override to evaluate ISA-specific gates
}

uint16_t Computer::get_pc() const
{
    return program_memory->get_selected_address();
}

void Computer::sync_pc()
{
    program_memory->evaluate();
}

uint16_t Computer::read_ram(uint16_t address) const
{
    return ram->get_register_value(address);
}

void Computer::read_pm_instruction(uint16_t address, uint16_t& opcode,
                                   uint16_t& a, uint16_t& b, uint16_t& c) const
{
    program_memory->get_instruction(address, opcode, a, b, c);
}

void Computer::write_pm_instruction(uint16_t address, uint16_t opcode,
                                    uint16_t a_val, uint16_t b_val, uint16_t c_val)
{
    uint16_t decoder_bits_     = program_memory->get_decoder_bits();
    uint16_t data_bits_        = program_memory->get_data_bits();
    uint16_t data_inputs_start = decoder_bits_;
    
    // Set address via signal generators
    for (uint16_t i = 0; i < decoder_bits_; ++i)
    {
        bool bit = ((address >> i) & 1) != 0;
        if (bit) (*pm_load_addr_sigs)[i].go_high();
        else     (*pm_load_addr_sigs)[i].go_low();
        (*pm_load_addr_sigs)[i].evaluate();
        program_memory->connect_input(&(*pm_load_addr_sigs)[i].get_outputs()[0], i);
    }
    
    // Set data (opcode, A, B, C) via signal generators
    uint16_t values[4] = { opcode, a_val, b_val, c_val };
    for (uint16_t reg = 0; reg < 4; ++reg)
    {
        for (uint16_t b = 0; b < data_bits_; ++b)
        {
            uint16_t idx = reg * data_bits_ + b;
            bool bit = ((values[reg] >> b) & 1) != 0;
            if (bit) (*pm_load_data_sigs)[idx].go_high();
            else     (*pm_load_data_sigs)[idx].go_low();
            (*pm_load_data_sigs)[idx].evaluate();
            program_memory->connect_input(
                &(*pm_load_data_sigs)[idx].get_outputs()[0],
                static_cast<uint16_t>(data_inputs_start + idx));
        }
    }
    
    // Pulse write enable
    pm_write_enable->go_high();
    pm_write_enable->evaluate();
    program_memory->evaluate();
    pm_write_enable->go_low();
    pm_write_enable->evaluate();
    
    // Restore PC outputs as PM address inputs
    bool* pc_outputs = cpu->get_pc_outputs();
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        program_memory->connect_input(&pc_outputs[i], i);
    }
    
    // Reconnect PM data inputs to permanent zeros to prevent stale write data
    // from corrupting subsequent writes
    for (uint16_t b = 0; b < 4 * data_bits_; ++b)
    {
        program_memory->connect_input(&(*pm_zero_sigs)[b].get_outputs()[0],
                                      static_cast<uint16_t>(data_inputs_start + b));
    }
}

void Computer::get_current_instruction(uint16_t& opcode, uint16_t& a,
                                       uint16_t& b, uint16_t& c) const
{
    bool* pm_out = program_memory->get_outputs();
    opcode = a = b = c = 0;
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        opcode |= (pm_out[i]               ? 1 : 0) << i;
        a      |= (pm_out[num_bits + i]     ? 1 : 0) << i;
        b      |= (pm_out[2 * num_bits + i] ? 1 : 0) << i;
        c      |= (pm_out[3 * num_bits + i] ? 1 : 0) << i;
    }
}

bool* Computer::get_cmp_flags() const
{
    if (cpu)
    {
        return cpu->get_cmp_flags();
    }
    return nullptr;
}

void Computer::prepare_run()
{
    // Ensure PM data inputs are connected (in case no write has happened yet)
    uint16_t decoder_bits_ = program_memory->get_decoder_bits();
    uint16_t data_bits_    = program_memory->get_data_bits();
    uint16_t data_start    = decoder_bits_;
    for (uint16_t b = 0; b < 4 * data_bits_; ++b)
    {
        program_memory->connect_input(&(*pm_zero_sigs)[b].get_outputs()[0],
                                      static_cast<uint16_t>(data_start + b));
    }
    
    // Reconnect PC to PM address inputs
    bool* pc_outputs = cpu->get_pc_outputs();
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        program_memory->connect_input(&pc_outputs[i], i);
    }
    program_memory->evaluate();
    is_running = true;
    execution_count = 0;
}

void Computer::_create_namestring(const std::string& name)
{
    // Create a unique component name string with memory address and optional name
    std::ostringstream oss;
    oss << "Computer 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
}

void Computer::_print_architecture_details() const
{
    // Print success message with architecture details
    std::cout << "\nComputer initialized" << std::endl;
    if (!computer_version.empty())
    {
        std::cout << "  Version: " << computer_version << std::endl;
    }
    if (!ISA_version.empty())
    {
        std::cout << "  ISA: " << ISA_version << std::endl;
    }
    std::cout << "  Data width: " << num_bits << " bits" << std::endl;
    std::cout << "  RAM addresses: " << num_ram_addresses << std::endl;
    std::cout << "  PM addresses: " << num_pm_addresses << "\n" << std::endl;
}
