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
      pm_write_enable(nullptr),
      pm_read_enable(nullptr),
      ram_write_enable(nullptr),
      ram_read_enable(nullptr),
      read_addr_high_low(nullptr),
      ram_write_or(nullptr),
      ram_read_flag(nullptr),
      ram_read_flag_not(nullptr),
      ram_we_gated(nullptr),
      ram_data_mux_not(nullptr),
      ram_data_mux_and_literal(nullptr),
      ram_data_mux_and_result(nullptr),
      ram_data_mux_or(nullptr),
      ram_write_addr_high_mux(nullptr),
      pm_load_addr_sigs(nullptr),
      pm_load_data_sigs(nullptr),
      ram_addr_sigs(nullptr),
      data_a_ptrs(nullptr),
      data_b_ptrs(nullptr),
      data_c_ptrs(nullptr),
      is_running(true),
      execution_count(0)
{
    // Signal generators for PM loading are sized here since pc_bits / num_bits
    // are already known. Subclass constructors do not need to create them.
    pm_load_addr_sigs = new std::vector<Signal_Generator>();
    pm_load_addr_sigs->resize(pc_bits);
    
    pm_load_data_sigs = new std::vector<Signal_Generator>();
    pm_load_data_sigs->resize(4 * num_bits);
    
    ram_addr_sigs = new std::vector<Signal_Generator>();
    ram_addr_sigs->resize(num_bits);
}

Computer::~Computer()
{
    delete cpu;
    delete program_memory;
    delete ram;
    delete pm_write_enable;
    delete pm_read_enable;
    delete ram_write_enable;
    delete ram_read_enable;
    delete read_addr_high_low;
    delete ram_write_or;
    delete ram_read_flag;
    delete ram_read_flag_not;
    delete ram_we_gated;
    delete ram_data_mux_not;
    
    delete[] data_a_ptrs;
    delete[] data_b_ptrs;
    delete[] data_c_ptrs;
    
    if (ram_data_mux_and_literal && ram_data_mux_and_result &&
        ram_data_mux_or && ram_write_addr_high_mux)
    {
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            delete ram_data_mux_and_literal[i];
            delete ram_data_mux_and_result[i];
            delete ram_data_mux_or[i];
            delete ram_write_addr_high_mux[i];
        }
    }
    delete[] ram_data_mux_and_literal;
    delete[] ram_data_mux_and_result;
    delete[] ram_data_mux_or;
    delete[] ram_write_addr_high_mux;
    
    delete pm_load_addr_sigs;
    delete pm_load_data_sigs;
    delete ram_addr_sigs;
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
        program_memory->update();
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

    for (auto& sig : *pm_load_data_sigs)
    {
        sig.go_low();
        sig.evaluate();
    }
    
    std::cout << "Loaded " << address << " instructions" << std::endl;
    file.close();
    return true;
}

void Computer::run_interactive()
{
    std::cout << "\n=== Starting Interactive Execution ===\n" << std::endl;
    std::cout << "Press Enter to execute each instruction..." << std::endl;
    
    program_memory->evaluate();
    
    bool running = true;
    while (running)
    {
        std::cout << "\nPress Enter to continue (or 'q' to quit): ";
        std::string input;
        std::getline(std::cin, input);
        
        if (input == "q" || input == "Q")
        {
            std::cout << "Execution stopped by user." << std::endl;
            break;
        }
        
        running = clock_tick();
        print_state();
        
        if (!running)
        {
            std::cout << "\n=== Program HALTED ===" << std::endl;
            break;
        }
    }
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
    
    // Phase 1: read flag HIGH → WE gated to 0, safe to read RAM
    toggle_ram_read_flag(true);
    
    ram->evaluate();   // combinational reads
    cpu->evaluate();   // compute next values
    
    // Evaluate write-path logic
    ram_data_mux_not->evaluate();
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        ram_data_mux_and_literal[i]->evaluate();
        ram_data_mux_and_result[i]->evaluate();
        ram_data_mux_or[i]->evaluate();
        ram_write_addr_high_mux[i]->evaluate();
    }
    
    // Phase 2: read flag LOW → WE follows ram_write_or
    ram_write_or->evaluate();
    toggle_ram_read_flag(false);
    
    ram->evaluate();   // latch writes
}

void Computer::update()
{
    evaluate();
    for (Component* downstream : downstream_components)
    {
        if (downstream) downstream->update();
    }
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
