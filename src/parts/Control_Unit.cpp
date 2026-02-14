#include "Control_Unit.hpp"
#include <sstream>
#include <iomanip>

// Delegating constructor: allow callers that omit explicit pc_bits to
// construct with (num_bits, opcode_bits, name). Default PC width will be
// twice the base `num_bits` (2 * num_bits) to match previous design.
Control_Unit::Control_Unit(uint16_t num_bits, uint16_t opcode_bits_param, const std::string& name)
    : Control_Unit(num_bits, opcode_bits_param, static_cast<uint16_t>(2 * num_bits), name)
{
}

Control_Unit::Control_Unit(uint16_t num_bits, uint16_t opcode_bits_param, uint16_t pc_bits_param, const std::string& name) : Part(num_bits, name)
{
    // create component name string
    std::ostringstream oss;
    oss << "Control_Unit 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty()) {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    // Allow override of PC bit width. If pc_bits_param > 0, use that, otherwise default to num_bits
    pc_bits = (pc_bits_param > 0) ? pc_bits_param : static_cast<uint16_t>(num_bits);
    opcode_bits = (opcode_bits_param == 0) ? num_bits : opcode_bits_param;  // Use param or default to num_bits
    num_flags = 6;  // Standard comparator flags: EQ, NEQ, LT_U, GT_U, LT_S, GT_S
    
    // === Program Counter and Incrementer ===
    pc = new Register(pc_bits, "pc_in_control_unit");
    // Zero-initialize PC outputs to 0
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        pc->get_outputs()[i] = false;
    }
    pc_incrementer = new Adder(pc_bits, "pc_incrementer_in_control_unit");
    
    // Create signal generators for increment value (binary 1: LSB=1, rest=0)
    increment_signals = new Signal_Generator*[pc_bits];
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        std::ostringstream name_str;
        name_str << "increment_signal_" << i << "_in_control_unit";
        increment_signals[i] = new Signal_Generator(name_str.str());
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
    jump_enable_inverter = new Inverter(1, "jump_enable_inverter_in_control_unit");
    pc_increment_and_gates = new AND_Gate*[pc_bits];
    pc_jump_and_gates = new AND_Gate*[pc_bits];
    pc_write_mux = new OR_Gate*[pc_bits];
    
    // Create default low signal for optional/unconnected inputs
    default_low_signal = new Signal_Generator("default_low_in_control_unit");
    default_low_signal->go_low();  // Sets output to false directly, no need to evaluate
    // Default: no jump enabled, no jump address
    jump_enable_inverter->connect_input(&default_low_signal->get_outputs()[0], 0);
    
    // === Run/Halt Flag Control ===
    // Halt flip-flop: controls whether PC increments
    // Q=1 means running, Q=0 means halted
    run_halt_flag = new Flip_Flop("run_halt_flag_in_control_unit");
    
    // Initialize to running state: Set=1, Reset=0
    // Pulse Set HIGH to initialize Q=1, then bring it back LOW
    halt_set_signal = new Signal_Generator("halt_set_signal_in_control_unit");
    halt_set_signal->go_high();  // Pulse Set=1
    halt_set_signal->evaluate();
    run_halt_flag->connect_input(&halt_set_signal->get_outputs()[0], 0);  // Set input

    // Halt OR gate: combines halt opcode signal with PC-carry signal
    // When either triggers (both are active-high), Reset the flip-flop (Q=0 stops execution)
    default_no_halt = new Signal_Generator("default_no_halt_in_control_unit");
    default_no_halt->go_low();  // Both inputs start LOW (no halt, no end-of-program)
    default_no_halt->evaluate();

    halt_or_gate = new OR_Gate(2, "halt_or_gate_in_control_unit");
    // Input 0: halt signal (from decoder via connect_halt_signal) - starts as default_no_halt (LOW)
    // Input 1: PC carry (for detecting end-of-program as a future feature)
    halt_or_gate->connect_input(&default_no_halt->get_outputs()[0], 0);
    halt_or_gate->connect_input(&default_no_halt->get_outputs()[0], 1);
    halt_or_gate->evaluate();
    
    // Halt inverter: provides immediate !halt signal for PC increment gating (no flip-flop latency)
    halt_inverter = new Inverter(1, "halt_inverter_in_control_unit");
    halt_inverter->connect_input(&halt_or_gate->get_outputs()[0], 0);
    halt_inverter->evaluate();

    // Connect halt trigger (OR of opcode and PC carry) to Reset input of flip-flop
    run_halt_flag->connect_input(&halt_or_gate->get_outputs()[0], 1);  // Reset input
    
    // Evaluate flip-flop with Set=HIGH, Reset=LOW to initialize Q=1
    run_halt_flag->evaluate();
    
    // Now bring Set back to LOW and keep it there during normal operation
    // During execution, only Reset will change (when HALT opcode fires)
    halt_set_signal->go_low();
    halt_set_signal->evaluate();
    
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        std::ostringstream and_inc_name, and_jmp_name, or_name;
        and_inc_name << "pc_increment_and_gate_" << i << "_in_control_unit";
        and_jmp_name << "pc_jump_and_gate_" << i << "_in_control_unit";
        or_name << "pc_write_mux_" << i << "_in_control_unit";
        
        // 3-input AND gate: incrementer & !jump_enable & run_flag
        pc_increment_and_gates[i] = new AND_Gate(3, and_inc_name.str());
        pc_jump_and_gates[i] = new AND_Gate(2, and_jmp_name.str());
        pc_write_mux[i] = new OR_Gate(2, or_name.str());
        
        // Wire: incrementer_output & !jump_enable & !halt -> AND gate
        pc_increment_and_gates[i]->connect_input(&pc_incrementer->get_outputs()[i], 0);
        pc_increment_and_gates[i]->connect_input(&jump_enable_inverter->get_outputs()[0], 1);
        pc_increment_and_gates[i]->connect_input(&halt_inverter->get_outputs()[0], 2);  // Immediate !halt signal (no latency)
        
        // Wire: jump_address & jump_enable -> OR gate
        // (jump address will be connected externally via connect_jump_address_to_pc)
        // Default: connect jump address to low (no jump)
        pc_jump_and_gates[i]->connect_input(&default_low_signal->get_outputs()[0], 0);
        // Default: connect jump enable to low (no jump)
        pc_jump_and_gates[i]->connect_input(&default_low_signal->get_outputs()[0], 1);
        
        // Wire OR outputs to PC data inputs
        pc_write_mux[i]->connect_input(&pc_increment_and_gates[i]->get_outputs()[0], 0);
        pc_write_mux[i]->connect_input(&pc_jump_and_gates[i]->get_outputs()[0], 1);
        pc->connect_input(&pc_write_mux[i]->get_outputs()[0], i);
    }
    
    // PC always writes and reads
    pc_write_enable = new Signal_Generator("pc_write_enable_in_control_unit");
    pc_write_enable->go_high();
    pc->connect_input(&pc_write_enable->get_outputs()[0], pc_bits);     // write enable
    
    pc_read_enable = new Signal_Generator("pc_read_enable_in_control_unit");
    pc_read_enable->go_high();
    pc->connect_input(&pc_read_enable->get_outputs()[0], pc_bits + 1);  // read enable
    
    // === Opcode Decoder ===
    opcode_decoder = new Decoder(opcode_bits, "opcode_decoder_in_control_unit");
    
    // === Comparator Flags Storage ===
    flag_register = new Register(num_flags, "flag_register_in_control_unit");
    flag_write_enable = new Signal_Generator("flag_write_enable_in_control_unit");
    flag_write_enable->go_high();  // Flags always write when provided
    flag_register->connect_input(&flag_write_enable->get_outputs()[0], num_flags);
    
    flag_read_enable = new Signal_Generator("flag_read_enable_in_control_unit");
    flag_read_enable->go_high();  // Flags always readable
    flag_register->connect_input(&flag_read_enable->get_outputs()[0], num_flags + 1);
    
    // Flag auto-clear mechanism: flip-flop tracks when to clear
    flag_clear_counter = new Flip_Flop("flag_clear_counter_in_control_unit");
    clear_set = new Signal_Generator("clear_set_in_control_unit");
    clear_set->go_low();  // Initially no clear
    clear_set->evaluate();
    clear_reset = new Signal_Generator("clear_reset_in_control_unit");
    clear_reset->go_low();  // Default no reset
    clear_reset->evaluate();
    
    // Connect Set/Reset inputs to flag_clear_counter
    flag_clear_counter->connect_input(&clear_set->get_outputs()[0], 0);  // Set input
    flag_clear_counter->connect_input(&clear_reset->get_outputs()[0], 1);  // Reset input
    
    clear_inverter = new Inverter(1, "clear_inverter_in_control_unit");
    
    // === RAM Page Register ===
    ram_page_register = new Register(pc_bits, "ram_page_register_in_control_unit");
    // Zero-initialize RAM page outputs to 0
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        ram_page_register->get_outputs()[i] = false;
    }
    ram_page_read_enable = new Signal_Generator("ram_page_read_enable_in_control_unit");
    ram_page_read_enable->go_high();
    ram_page_register->connect_input(&ram_page_read_enable->get_outputs()[0], pc_bits + 1);
    // Ensure RAM page register write-enable defaults to low when not driven
    ram_page_register->connect_input(&default_low_signal->get_outputs()[0], pc_bits);
    // Also default all RAM page data inputs to low to avoid unconnected memory bit errors
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        ram_page_register->connect_input(&default_low_signal->get_outputs()[0], i);
    }
    
    // === Stack Pointer and Return Address === DISABLED: inputs not connected
    //stack_pointer = new Register(pc_bits, "stack_pointer_in_control_unit");
    //// Zero-initialize stack pointer outputs to 0
    //for (uint16_t i = 0; i < pc_bits; ++i)
    //{
    //    stack_pointer->get_outputs()[i] = false;
    //}
    //sp_read_enable = new Signal_Generator("sp_read_enable_in_control_unit");
    //sp_read_enable->go_high();
    //stack_pointer->connect_input(&sp_read_enable->get_outputs()[0], pc_bits + 1);
    //
    //return_address = new Register(pc_bits, "return_address_in_control_unit");
    //ra_read_enable = new Signal_Generator("ra_read_enable_in_control_unit");
    //ra_read_enable->go_high();
    //return_address->connect_input(&ra_read_enable->get_outputs()[0], pc_bits + 1);
}

Control_Unit::~Control_Unit()
{
    delete pc;
    delete pc_incrementer;
    
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        delete increment_signals[i];
        delete pc_increment_and_gates[i];
        delete pc_jump_and_gates[i];
        delete pc_write_mux[i];
    }
    delete[] increment_signals;
    delete[] pc_increment_and_gates;
    delete[] pc_jump_and_gates;
    delete[] pc_write_mux;
    
    delete jump_enable_inverter;
    delete default_low_signal;
    delete pc_write_enable;
    delete pc_read_enable;
    delete opcode_decoder;
    delete flag_register;
    delete flag_write_enable;
    delete flag_read_enable;
    delete flag_clear_counter;
    delete clear_set;
    delete clear_reset;
    delete clear_inverter;
    delete ram_page_register;
    delete ram_page_read_enable;
    delete run_halt_flag;
    delete halt_set_signal;
    delete halt_or_gate;
    delete halt_inverter;
    delete default_no_halt;
    //delete stack_pointer;  // DISABLED
    //delete sp_read_enable;  // DISABLED
    //delete return_address;  // DISABLED
    //delete ra_read_enable;  // DISABLED
}

void Control_Unit::evaluate()
{
    // Evaluate in dependency order
    
    // 0. Evaluate run/halt flag and halt detection (used in PC increment gating)
    halt_or_gate->evaluate();
    halt_inverter->evaluate();  // Immediate halt signal for PC gating
    run_halt_flag->evaluate();
    
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
        pc_increment_and_gates[i]->evaluate();
        pc_jump_and_gates[i]->evaluate();
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
    //sp_read_enable->evaluate();  // DISABLED: inputs not connected
    //stack_pointer->evaluate();  // DISABLED: inputs not connected
    //ra_read_enable->evaluate();  // DISABLED: inputs not connected
    //return_address->evaluate();  // DISABLED: inputs not connected
}

void Control_Unit::update()
{
    // DO NOT CALL EVALUATE HERE!
    // update() should only latch storage elements, not re-compute combinational logic
    
    // Update internal storage elements (flip-flops and registers)
    run_halt_flag->update();
    flag_register->update();
    flag_clear_counter->update();
    pc->update();
    ram_page_register->update();
    
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
        pc_jump_and_gates[i]->connect_input(jump_address_output[i], 0);
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
        pc_jump_and_gates[i]->connect_input(jump_enable_signal, 1);
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

bool Control_Unit::connect_pc_carry(const bool* carry_signal)
{
    if (!carry_signal)
        return false;

    // Connect the carry output to the halt OR gate input (PC overflow triggers halt)
    halt_or_gate->connect_input(carry_signal, 1);
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

//bool* Control_Unit::get_stack_pointer_outputs() const
//{
//    return stack_pointer->get_outputs();
//}  // DISABLED: inputs not connected

bool Control_Unit::get_run_halt_flag() const
{
    return run_halt_flag->get_outputs()[0];  // Q output
}

void Control_Unit::set_run_halt_flag(bool state)
{
    if (state)
    {
        // Set to running: Q = 1 (reset flip-flop, since reset is active low)
        halt_set_signal->go_high();
    }
    else
    {
        // Set to halted: Q = 0 (set flip-flop)
        halt_set_signal->go_low();
    }
    halt_set_signal->evaluate();
    run_halt_flag->connect_input(&halt_set_signal->get_outputs()[0], 0);
    run_halt_flag->evaluate();
}

bool Control_Unit::connect_halt_signal(const bool* halt_signal)
{
    if (!halt_signal)
        return false;
    
    // Connect halt signal directly to halt_or_gate input 0
    // When halt opcode is decoded, halt_signal goes HIGH, triggering Reset (Q=0)
    halt_or_gate->connect_input(halt_signal, 0);
    
    return true;
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

