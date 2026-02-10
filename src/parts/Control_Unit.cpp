#include "Control_Unit.hpp"
#include <sstream>
#include <iomanip>

Control_Unit::Control_Unit(uint16_t num_bits) : Part(num_bits)
{
    // create component name string
    std::ostringstream oss;
    oss << "Control_Unit 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    
    pc_bits = static_cast<uint16_t>(2 * num_bits);
    opcode_bits = num_bits;  // Default opcode size = num_bits
    num_flags = 6;  // Standard comparator flags: EQ, NEQ, LT_U, GT_U, LT_S, GT_S
    
    // === Program Counter and Incrementer ===
    pc = new Register(pc_bits);
    pc_incrementer = new Adder(pc_bits);
    
    // Create signal generators for increment value (binary 1: LSB=1, rest=0)
    increment_signals = new Signal_Generator*[pc_bits];
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        increment_signals[i] = new Signal_Generator();
        if (i == 0)
            increment_signals[i]->go_high();  // LSB = 1
        else
            increment_signals[i]->go_low();   // All other bits = 0
        
        increment_signals[i]->evaluate();
        pc_incrementer->connect_input(&increment_signals[i]->get_outputs()[0], i);  // A inputs = 1
    }
    
    // Connect PC output to incrementer B input
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        pc_incrementer->connect_input(&pc->get_outputs()[i], pc_bits + i); // B inputs = PC
    }
    
    // === PC Write Control (Mux between increment and jump) ===
    jump_enable_inverter = new Inverter();
    pc_increment_gates = new AND_Gate*[pc_bits];
    pc_jump_gates = new AND_Gate*[pc_bits];
    pc_write_mux = new OR_Gate*[pc_bits];
    
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        pc_increment_gates[i] = new AND_Gate(2);
        pc_jump_gates[i] = new AND_Gate(2);
        pc_write_mux[i] = new OR_Gate(2);
        
        // Wire: incrementer_output & !jump_enable -> OR gate
        pc_increment_gates[i]->connect_input(&pc_incrementer->get_outputs()[i], 0);
        pc_increment_gates[i]->connect_input(&jump_enable_inverter->get_outputs()[0], 1);
        
        // Wire: jump_address & jump_enable -> OR gate
        // (jump address will be connected externally via connect_jump_address_to_pc)
        // pc_jump_gates wiring done in connect methods
        
        // Wire OR outputs to PC data inputs
        pc_write_mux[i]->connect_input(&pc_increment_gates[i]->get_outputs()[0], 0);
        pc_write_mux[i]->connect_input(&pc_jump_gates[i]->get_outputs()[0], 1);
        pc->connect_input(&pc_write_mux[i]->get_outputs()[0], i);
    }
    
    // PC always writes and reads
    pc_write_enable = new Signal_Generator();
    pc_write_enable->go_high();
    pc->connect_input(&pc_write_enable->get_outputs()[0], pc_bits);     // write enable
    
    pc_read_enable = new Signal_Generator();
    pc_read_enable->go_high();
    pc->connect_input(&pc_read_enable->get_outputs()[0], pc_bits + 1);  // read enable
    
    // === Opcode Decoder ===
    opcode_decoder = new Decoder(opcode_bits);
    
    // === Comparator Flags Storage ===
    flag_register = new Register(num_flags);
    flag_write_enable = new Signal_Generator();
    flag_write_enable->go_high();  // Flags always write when provided
    flag_register->connect_input(&flag_write_enable->get_outputs()[0], num_flags);
    
    flag_read_enable = new Signal_Generator();
    flag_read_enable->go_high();  // Flags always readable
    flag_register->connect_input(&flag_read_enable->get_outputs()[0], num_flags + 1);
    
    // Flag auto-clear mechanism: flip-flop tracks when to clear
    flag_clear_counter = new Flip_Flop();
    clear_set = new Signal_Generator();
    clear_set->go_low();  // Initially no clear
    clear_inverter = new Inverter();
    
    // === RAM Page Register ===
    ram_page_register = new Register(pc_bits);
    ram_page_read_enable = new Signal_Generator();
    ram_page_read_enable->go_high();
    ram_page_register->connect_input(&ram_page_read_enable->get_outputs()[0], pc_bits + 1);
    
    // === Stack Pointer and Return Address ===
    stack_pointer = new Register(pc_bits);
    sp_read_enable = new Signal_Generator();
    sp_read_enable->go_high();
    stack_pointer->connect_input(&sp_read_enable->get_outputs()[0], pc_bits + 1);
    
    return_address = new Register(pc_bits);
    ra_read_enable = new Signal_Generator();
    ra_read_enable->go_high();
    return_address->connect_input(&ra_read_enable->get_outputs()[0], pc_bits + 1);
}

Control_Unit::~Control_Unit()
{
    delete pc;
    delete pc_incrementer;
    
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        delete increment_signals[i];
        delete pc_increment_gates[i];
        delete pc_jump_gates[i];
        delete pc_write_mux[i];
    }
    delete[] increment_signals;
    delete[] pc_increment_gates;
    delete[] pc_jump_gates;
    delete[] pc_write_mux;
    
    delete jump_enable_inverter;
    delete pc_write_enable;
    delete pc_read_enable;
    delete opcode_decoder;
    delete flag_register;
    delete flag_write_enable;
    delete flag_read_enable;
    delete flag_clear_counter;
    delete clear_set;
    delete clear_inverter;
    delete ram_page_register;
    delete ram_page_read_enable;
    delete stack_pointer;
    delete sp_read_enable;
    delete return_address;
    delete ra_read_enable;
}

void Control_Unit::evaluate()
{
    // Evaluate in dependency order
    
    // 1. Evaluate increment signals and PC incrementer (depends on PC output)
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        increment_signals[i]->evaluate();
    }
    pc_incrementer->evaluate();
    
    // 2. Jump control logic
    jump_enable_inverter->evaluate();
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        pc_increment_gates[i]->evaluate();
        pc_jump_gates[i]->evaluate();
        pc_write_mux[i]->evaluate();
    }
    
    // 3. Update PC
    pc->evaluate();
    
    // 4. Decode opcode
    opcode_decoder->evaluate();
    
    // 5. Evaluate flags
    flag_write_enable->evaluate();
    flag_read_enable->evaluate();
    flag_register->evaluate();
    
    // 6. Evaluate other registers
    ram_page_read_enable->evaluate();
    ram_page_register->evaluate();
    sp_read_enable->evaluate();
    stack_pointer->evaluate();
    ra_read_enable->evaluate();
    return_address->evaluate();
}

void Control_Unit::update()
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

bool Control_Unit::connect_pc_to_pm_address(bool** pm_address_input, uint16_t start_index)
{
    if (!pm_address_input)
        return false;
    
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        pm_address_input[start_index + i] = &pc->get_outputs()[i];
    }
    return true;
}

bool Control_Unit::connect_jump_address_to_pc(const bool* const* jump_address_output, uint16_t num_address_bits)
{
    if (!jump_address_output || num_address_bits != pc_bits)
        return false;
    
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        pc_jump_gates[i]->connect_input(jump_address_output[i], 0);
    }
    return true;
}

bool Control_Unit::connect_jump_enable(const bool* jump_enable_signal)
{
    if (!jump_enable_signal)
        return false;
    
    // Connect to inverter
    jump_enable_inverter->connect_input(jump_enable_signal, 0);
    
    // Connect to all jump gates
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        pc_jump_gates[i]->connect_input(jump_enable_signal, 1);
    }
    
    return true;
}

bool Control_Unit::connect_opcode_input(const bool* const* opcode_output, uint16_t opcode_bit_count)
{
    if (!opcode_output || opcode_bit_count != opcode_bits)
        return false;
    
    for (uint16_t i = 0; i < opcode_bits; ++i)
    {
        opcode_decoder->connect_input(opcode_output[i], i);
    }
    return true;
}

bool Control_Unit::connect_comparator_flags(const bool* const* flag_outputs, uint16_t num_flag_signals)
{
    if (!flag_outputs || num_flag_signals != num_flags)
        return false;
    
    for (uint16_t i = 0; i < num_flags; ++i)
    {
        flag_register->connect_input(flag_outputs[i], i);
    }
    return true;
}

bool Control_Unit::connect_ram_page_data_input(const bool* const* page_data_input, uint16_t num_page_bits)
{
    if (!page_data_input || num_page_bits != pc_bits)
        return false;
    
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        ram_page_register->connect_input(page_data_input[i], i);
    }
    return true;
}

bool Control_Unit::connect_ram_page_write_enable(const bool* page_write_enable)
{
    if (!page_write_enable)
        return false;
    
    ram_page_register->connect_input(page_write_enable, pc_bits);
    return true;
}

bool* Control_Unit::get_pc_outputs() const
{
    return pc->get_outputs();
}

bool* Control_Unit::get_decoder_outputs() const
{
    return opcode_decoder->get_outputs();
}

bool* Control_Unit::get_stored_flags() const
{
    return flag_register->get_outputs();
}

bool* Control_Unit::get_ram_page_outputs() const
{
    return ram_page_register->get_outputs();
}

bool* Control_Unit::get_stack_pointer_outputs() const
{
    return stack_pointer->get_outputs();
}

void Control_Unit::clock_tick()
{
    // Clock tick: used for sequential logic like flag clearing
    // On each tick, we set the clear counter, which will trigger flag clear on next evaluation
    
    // Set the flip-flop to trigger clear
    clear_set->go_high();
    clear_set->evaluate();
    flag_clear_counter->connect_input(&clear_set->get_outputs()[0], 0);  // Set
    
    Signal_Generator reset_low;
    reset_low.go_low();
    reset_low.evaluate();
    flag_clear_counter->connect_input(&reset_low.get_outputs()[0], 1);  // Reset = 0
    flag_clear_counter->evaluate();
    
    // After one cycle, clear the set signal
    clear_set->go_low();
}

