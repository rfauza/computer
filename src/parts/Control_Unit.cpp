#include "Control_Unit.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

// Delegating constructor: allow callers that omit explicit pc_bits to
// construct with (num_bits, opcode_bits, name). Default PC width will be
// twice the base `num_bits` (2 * num_bits) to match previous design.
Control_Unit::Control_Unit(uint16_t num_bits, uint16_t opcode_bits_param, const std::string& name)
    : Control_Unit(num_bits, opcode_bits_param, static_cast<uint16_t>(2 * num_bits), name)
{
}

Control_Unit::Control_Unit(uint16_t num_bits, uint16_t opcode_bits_param, uint16_t pc_bits_param, const std::string& name) : Part(num_bits, name)
{
    // === Create component name string ===
    std::ostringstream oss;
    oss << "Control_Unit 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty()) {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    // === Jump Instruction Conditions === (initialized in connect_jump_instructions)
    jump_instruction_and_gates = nullptr;
    jump_instructions_or_gate = nullptr;
    num_jump_conditions = 0;
    
    // Expect pc_bits_param = 3 * numbits. set to numbits otherwise
    // Allow override of PC bit width. If pc_bits_param > 0, use that, otherwise default to num_bits
    pc_bits = (pc_bits_param > 0) ? pc_bits_param : static_cast<uint16_t>(num_bits);
    num_cmp_flags = 6;  // Standard comparator flags: EQ, NEQ, LT_U, GT_U, LT_S, GT_S
    // === Opcode Decoder ===
    opcode_bits = (opcode_bits_param == 0) ? num_bits : opcode_bits_param;  // Use param or default to num_bits
    opcode_decoder = new Decoder(opcode_bits, "opcode_decoder_in_control_unit");
    
    _create_program_counter();
    _setup_run_halt_logic();
    _setup_compare_flags();
    _setup_rampage(); // TODO
    _setup_opcode_page(); // TODO
    _setup_function_calling(); // TODO
}

void Control_Unit::_create_program_counter()
{
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
        
        // increment_signals[i]->evaluate();
        pc_incrementer->connect_input(&pc->get_outputs()[i], i); // A inputs = PC output
        pc_incrementer->connect_input(&increment_signals[i]->get_outputs()[0], pc_bits + i);  // B inputs = 0001
    }
    
    // === PC Write Control (Mux between halt-gated increment and jump address) ===
    jump_enable_inverter = new Inverter(1, "jump_enable_inverter_in_control_unit");
    pc_halt_and_gates = new AND_Gate*[pc_bits];
    pc_write_mux = new Multiplexer(pc_bits, 2, "pc_write_mux_in_control_unit");
    
    // Create default low signal for optional/unconnected inputs
    default_low_signal = new Signal_Generator("default_low_in_control_unit");
    default_low_signal->go_low();  // Sets output to false directly, no need to evaluate
    // Default: no jump enabled, no jump address
    jump_enable_inverter->connect_input(&default_low_signal->get_outputs()[0], 0);
    
    // Create persistent signal generators for set_pc operations
    pc_set_addr_sigs = new std::vector<Signal_Generator>();
    pc_set_addr_sigs->reserve(pc_bits);
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        std::ostringstream name_str;
        name_str << "pc_set_addr_signal_" << i << "_in_control_unit";
        pc_set_addr_sigs->emplace_back(name_str.str());
    }
}

void Control_Unit::_setup_run_halt_logic()
{
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
        std::ostringstream and_halt_name;
        and_halt_name << "pc_halt_and_gate_" << i << "_in_control_unit";
        
        // 2-input AND gate: incrementer[i] & !halt
        pc_halt_and_gates[i] = new AND_Gate(2, and_halt_name.str());
        pc_halt_and_gates[i]->connect_input(&pc_incrementer->get_outputs()[i], 0);
        pc_halt_and_gates[i]->connect_input(&halt_inverter->get_outputs()[0], 1);
        
        // Source[0]: halt-gated increment; source[1]: jump address (default low)
        pc_write_mux->connect_source_data_bit(0, i, &pc_halt_and_gates[i]->get_outputs()[0]);
        pc_write_mux->connect_source_data_bit(1, i, &default_low_signal->get_outputs()[0]);
        
        // Wire mux output to PC data input
        pc->connect_input(&pc_write_mux->get_outputs()[i], i);
    }
    // Source controls: [0]=!jump_enable, [1]=low by default (updated by connect_jump_enable)
    pc_write_mux->connect_source_control(0, &jump_enable_inverter->get_outputs()[0]);
    pc_write_mux->connect_source_control(1, &default_low_signal->get_outputs()[0]);
    
    // PC always writes and reads
    pc_write_enable = new Signal_Generator("pc_write_enable_in_control_unit");
    pc_write_enable->go_high();
    pc->connect_input(&pc_write_enable->get_outputs()[0], pc_bits);     // write enable

    pc_read_enable = new Signal_Generator("pc_read_enable_in_control_unit");
    pc_read_enable->go_high();
    pc->connect_input(&pc_read_enable->get_outputs()[0], pc_bits + 1);  // read enable
}

void Control_Unit::_setup_compare_flags()
{
    // === Comparator Flags Storage ===
    flag_register = new Register(num_cmp_flags, "flag_register_in_control_unit");
    flag_write_enable = new Signal_Generator("flag_write_enable_in_control_unit");
    flag_write_enable->go_low();  // Flags write enabled by cmp decoder output, so default to low
    flag_register->connect_input(&flag_write_enable->get_outputs()[0], num_cmp_flags);
    
    flag_read_enable = new Signal_Generator("flag_read_enable_in_control_unit");
    flag_read_enable->go_high();  // Flags always readable
    flag_register->connect_input(&flag_read_enable->get_outputs()[0], num_cmp_flags + 1);
    
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
    
}

void Control_Unit::_setup_rampage()
{
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
    
}

void Control_Unit::_setup_opcode_page()
{
    
}

void Control_Unit::_setup_function_calling()
{
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
        delete pc_halt_and_gates[i];
    }
    delete[] increment_signals;
    delete[] pc_halt_and_gates;
    delete pc_write_mux;
    delete jump_enable_inverter;
    delete default_low_signal;
    delete pc_set_addr_sigs;
    delete pc_write_enable;
    delete pc_read_enable;
    delete opcode_decoder;
    delete flag_register;
    delete flag_write_enable;
    delete flag_read_enable;
    delete flag_clear_counter;
    delete clear_set;
    delete clear_reset;
    delete ram_page_register;
    delete ram_page_read_enable;
    delete run_halt_flag;
    delete halt_set_signal;
    delete halt_or_gate;
    delete halt_inverter;
    delete default_no_halt;
    
    // Clean up jump instruction gates
    if (jump_instruction_and_gates)
    {
        for (uint16_t i = 0; i < num_jump_conditions; ++i)
        {
            if (jump_instruction_and_gates[i])
                delete jump_instruction_and_gates[i];
        }
        delete[] jump_instruction_and_gates;
    }
    if (jump_instructions_or_gate)
        delete jump_instructions_or_gate;
    
    //delete stack_pointer;  // DISABLED
    //delete sp_read_enable;  // DISABLED
    //delete return_address;  // DISABLED
    //delete ra_read_enable;  // DISABLED
}

void Control_Unit::evaluate()
{
    // Evaluate in dependency order
    
    // 0. Decode opcode early so decoder outputs are current for ALL downstream logic
    opcode_decoder->evaluate();

    // 0.5. Flag register is evaluated by CPU::evaluate() AFTER alu->evaluate(),
    //      so comparator outputs are fresh. See CPU::evaluate_flag_register().

    // 1. Evaluate run/halt flag and halt detection (used in PC increment gating)
    halt_or_gate->evaluate();
    halt_inverter->evaluate();  // Immediate halt signal for PC gating
    run_halt_flag->evaluate();
    
    // 1. Evaluate increment signals and PC incrementer (depends on PC output)
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        increment_signals[i]->evaluate();
    }
    pc_incrementer->evaluate();
    
    // 1.5. Evaluate jump instruction conditions (jump instructions ANDed with flags)
    if (jump_instruction_and_gates && num_jump_conditions > 0)
    {
        for (uint16_t i = 0; i < num_jump_conditions; ++i)
        {
            jump_instruction_and_gates[i]->evaluate();
        }
        if (jump_instructions_or_gate)
        {
            jump_instructions_or_gate->evaluate();
        }
    }
    
    // 2. Jump control logic
    jump_enable_inverter->evaluate();
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        pc_halt_and_gates[i]->evaluate();
    }
    pc_write_mux->evaluate();
    
    // 3. Update PC
    pc->evaluate();
    
    // 4. (Already done at top) Decode opcode
    // opcode_decoder->evaluate();
    
    // 6. Evaluate other registers
    ram_page_read_enable->evaluate();
    ram_page_register->evaluate();
    //sp_read_enable->evaluate();  // DISABLED: inputs not connected
    //stack_pointer->evaluate();  // DISABLED: inputs not connected
    //ra_read_enable->evaluate();  // DISABLED: inputs not connected
    //return_address->evaluate();  // DISABLED: inputs not connected
}

// void Control_Unit::update()
// {
//     /* intentionally skips recomputing combinational logic and instead only latches sequential/storage elements 
//     (flip‑flops and registers) then propagates updates to downstream parts. That preserves the intended two-phase model */
    
//     // update() should only latch storage elements, not re-compute combinational logic
//     // Update internal storage elements (flip-flops and registers)
//     run_halt_flag->update();
//     flag_register->update();
//     flag_clear_counter->update();
//     pc->update();
//     ram_page_register->update();
    
//     // Signal all downstream components to update
//     for (Component* downstream : downstream_components)
//     {
//         if (downstream)
//         {
//             downstream->update();
//         }
//     }
// }

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
        pc_write_mux->connect_source_data_bit(1, i, jump_address_output[i]);
    }
    return true;
}

bool Control_Unit::connect_jump_enable(const bool* jump_enable_signal)
{
    if (!jump_enable_signal)
        return false;
    
    // Connect to inverter
    jump_enable_inverter->connect_input(jump_enable_signal, 0);
    
    // Update jump source control in the mux
    pc_write_mux->connect_source_control(1, jump_enable_signal);
    
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
    if (!flag_outputs || num_flag_signals != num_cmp_flags)
        return false;
    
    for (uint16_t i = 0; i < num_cmp_flags; ++i)
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

void Control_Unit::evaluate_flag_register()
{
    // Called by CPU::evaluate() after ALU runs so comparator outputs are fresh
    flag_write_enable->evaluate();
    flag_read_enable->evaluate();
    flag_register->evaluate();
}

bool Control_Unit::connect_flag_write_enable(const bool* signal_ptr)
{
    if (!signal_ptr)
        return false;
    
    // Directly reconnect the flag register's write input to the decoder output signal
    // This bypasses the flag_write_enable Signal_Generator and wires the signal directly
    flag_register->connect_input(signal_ptr, num_cmp_flags);
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

bool* Control_Unit::get_cmp_flags() const
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
        // Pulse Set HIGH to force Q=1 (running), then bring it back LOW so that
        // a future HALT instruction can pull Q=0 without the forbidden S=1,R=1 state.
        halt_set_signal->go_high();
        halt_set_signal->evaluate();
        run_halt_flag->connect_input(&halt_set_signal->get_outputs()[0], 0);
        run_halt_flag->evaluate();
        halt_set_signal->go_low();
        halt_set_signal->evaluate();
    }
    else
    {
        // Set to halted: Q = 0
        halt_set_signal->go_low();
        halt_set_signal->evaluate();
        run_halt_flag->connect_input(&halt_set_signal->get_outputs()[0], 0);
        run_halt_flag->evaluate();
    }
}

void Control_Unit::reset_pc()
{
    // Temporarily connect all PC data inputs to the always-low signal,
    // evaluate to latch zeros, then restore the mux outputs.
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        pc->connect_input(&default_low_signal->get_outputs()[0], i);
    }
    pc->evaluate();
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        pc->connect_input(&pc_write_mux->get_outputs()[i], i);
    }
}

void Control_Unit::set_pc(uint16_t address)
{
    // Set PC to the specified address using persistent signal generators.
    // Set each signal to match the corresponding address bit
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        bool bit = ((address >> i) & 1) != 0;
        if (bit)
        {
            (*pc_set_addr_sigs)[i].go_high();
        }
        else
        {
            (*pc_set_addr_sigs)[i].go_low();
        }
        (*pc_set_addr_sigs)[i].evaluate();
        pc->connect_input(&(*pc_set_addr_sigs)[i].get_outputs()[0], i);
    }

    // Evaluate the PC register to latch the new address
    pc->evaluate();

    // Restore the mux outputs
    for (uint16_t i = 0; i < pc_bits; ++i)
    {
        pc->connect_input(&pc_write_mux->get_outputs()[i], i);
    }
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

bool Control_Unit::connect_jump_instructions(const std::vector<std::pair<uint16_t, uint16_t>>& jump_conditions)
{
    if (jump_conditions.empty())
        return false;
    
    // Store number of jump conditions
    num_jump_conditions = static_cast<uint16_t>(jump_conditions.size());
    
    // Create AND gates for each jump condition (jump_instruction & flag)
    jump_instruction_and_gates = new AND_Gate*[num_jump_conditions];
    for (uint16_t i = 0; i < num_jump_conditions; ++i)
    {
        std::ostringstream and_name;
        and_name << "jump_instruction_and_" << i << "_in_control_unit";
        jump_instruction_and_gates[i] = new AND_Gate(2, and_name.str());
        
        // Get decoder and flag indices for this jump condition
        uint16_t decoder_index = jump_conditions[i].first;
        uint16_t flag_index = jump_conditions[i].second;
        
        // Validate flag index
        if (flag_index >= num_cmp_flags)
            return false;
        
        // Connect decoder output (jump instruction signal) to AND gate input 0
        jump_instruction_and_gates[i]->connect_input(&opcode_decoder->get_outputs()[decoder_index], 0);
        
        // Connect comparator flag to AND gate input 1
        jump_instruction_and_gates[i]->connect_input(&flag_register->get_outputs()[flag_index], 1);
    }
    
    // Create OR gate to combine all jump conditions
    jump_instructions_or_gate = new OR_Gate(num_jump_conditions, "jump_instructions_or_gate_in_control_unit");
    
    // Connect all AND gate outputs to the OR gate
    for (uint16_t i = 0; i < num_jump_conditions; ++i)
    {
        jump_instructions_or_gate->connect_input(&jump_instruction_and_gates[i]->get_outputs()[0], i);
    }
    
    // Connect OR gate output to jump_enable signal
    connect_jump_enable(&jump_instructions_or_gate->get_outputs()[0]);
    
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

