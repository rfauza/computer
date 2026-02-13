#include "Divider_Sequential.hpp"
#include <sstream>
#include <iomanip>

Divider_Sequential::Divider_Sequential(uint16_t num_bits, const std::string& name) : Device(num_bits, name), cycle_count(0)
{
    std::ostringstream oss;
    oss << "Divider_Sequential 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    
    num_inputs = 2 * num_bits + 2;  // dividend (num_bits) + divisor (num_bits) + start + output_enable
    num_outputs = 2 * num_bits + 1; // quotient (num_bits) + remainder (num_bits) + busy
    
    allocate_IO_arrays();

    // Initialize outputs to false to avoid uninitialized values
    for (uint16_t i = 0; i < num_outputs; ++i)
    {
        outputs[i] = false;
    }
    
    // Create registers
    quotient = new Register(num_bits, "quotient_in_divider_sequential");
    remainder = new Register(num_bits, "remainder_in_divider_sequential");
    divisor = new Register(num_bits, "divisor_in_divider_sequential");
    busy_flag = new Flip_Flop("busy_flag_in_divider_sequential");
    
    // Create combinational components
    subtractor = new Adder_Subtractor(num_bits, "subtractor_in_divider_sequential");
    shift_left_rem = new L_Shift(num_bits, "shift_left_rem_in_divider_sequential");
    shift_left_quot = new L_Shift(num_bits, "shift_left_quot_in_divider_sequential");
    
    // Create control signals
    write_enable = new Signal_Generator("write_enable_in_divider_sequential");
    read_enable = new Signal_Generator("read_enable_in_divider_sequential");
    zero_signal = new Signal_Generator("zero_signal_in_divider_sequential");
    one_signal = new Signal_Generator("one_signal_in_divider_sequential");
    
    write_enable->go_low();
    read_enable->go_high();
    zero_signal->go_low();
    one_signal->go_high();
    
    // Initialize output_enable pointer
    output_enable = nullptr;
    
    // Allocate AND gates for output gating
    output_AND_gates = new AND_Gate*[2 * num_bits];
    std::ostringstream output_and_name;
    for (uint16_t i = 0; i < 2 * num_bits; ++i)
    {
        output_and_name.str("");
        output_and_name.clear();
        output_and_name << "output_and_" << i << "_in_divider_sequential";
        output_AND_gates[i] = new AND_Gate(2, output_and_name.str());
    }
    
    // Connect quotient and remainder outputs to AND gate input 0
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        output_AND_gates[i]->connect_input(&quotient->get_outputs()[i], 0);
        output_AND_gates[num_bits + i]->connect_input(&remainder->get_outputs()[i], 0);
    }
    
    // Allocate dividend bit storage
    dividend_bits = new bool[num_bits];
    
    // Connect read enables (always high for combinational access)
    quotient->connect_input(&read_enable->get_outputs()[0], num_bits + 1);
    remainder->connect_input(&read_enable->get_outputs()[0], num_bits + 1);
    divisor->connect_input(&read_enable->get_outputs()[0], num_bits + 1);
    
    // Connect subtractor's subtract enable (always subtracting)
    subtractor->connect_input(&one_signal->get_outputs()[0], 2 * num_bits);
    
    // Busy flag output gets direct pointer
    outputs[2 * num_bits] = &busy_flag->get_outputs()[0];  // Busy flag output
}

Divider_Sequential::~Divider_Sequential()
{
    for (uint16_t i = 0; i < 2 * num_bits; ++i)
    {
        delete output_AND_gates[i];
    }
    delete[] output_AND_gates;
    delete quotient;
    delete remainder;
    delete divisor;
    delete busy_flag;
    delete subtractor;
    delete shift_left_rem;
    delete shift_left_quot;
    delete write_enable;
    delete read_enable;
    delete zero_signal;
    delete one_signal;
    delete[] dividend_bits;
}

bool Divider_Sequential::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Dividend inputs: 0 to num_bits-1
    // Divisor inputs: num_bits to 2*num_bits-1
    // Start signal: 2*num_bits
    // Output enable: 2*num_bits+1
    
    if (input_index == 2 * num_bits + 1)
    {
        // Output enable: route to all AND gate input 1
        output_enable = inputs[input_index];
        bool result = true;
        for (uint16_t i = 0; i < 2 * num_bits; ++i)
        {
            result &= output_AND_gates[i]->connect_input(output_enable, 1);
        }
        return result;
    }
    
    return true;
}

void Divider_Sequential::start()
{
    write_enable->go_high();
    
    // Save dividend bits for later use
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        dividend_bits[i] = *inputs[i];
    }
    
    // Load divisor into register
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        divisor->connect_input(inputs[num_bits + i], i);
    }
    divisor->connect_input(&write_enable->get_outputs()[0], num_bits);
    divisor->update();
    
    // Clear remainder
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        remainder->connect_input(&zero_signal->get_outputs()[0], i);
    }
    remainder->connect_input(&write_enable->get_outputs()[0], num_bits);
    remainder->update();
    
    // Clear quotient
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        quotient->connect_input(&zero_signal->get_outputs()[0], i);
    }
    quotient->connect_input(&write_enable->get_outputs()[0], num_bits);
    quotient->update();
    
    // Set busy flag
    busy_flag->connect_input(&one_signal->get_outputs()[0], 0);  // Set
    busy_flag->connect_input(&zero_signal->get_outputs()[0], 1);  // Reset (inactive)
    busy_flag->update();
    
    write_enable->go_low();
    cycle_count = 0;
}

bool Divider_Sequential::is_busy() const
{
    return busy_flag->get_outputs()[0];
}

void Divider_Sequential::evaluate()
{
    if (is_busy())
    {
    
    write_enable->go_high();
    
    // Step 1: Shift remainder left, bring in next bit of dividend (MSB first)
    uint16_t dividend_bit_index = num_bits - 1 - cycle_count;
    
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        shift_left_rem->connect_input(&remainder->get_outputs()[i], i);
    }
    shift_left_rem->connect_input(&write_enable->get_outputs()[0], num_bits);  // Enable
    shift_left_rem->evaluate();
    
    // Store shifted remainder with new dividend bit in LSB
    for (uint16_t i = 1; i < num_bits; ++i)
    {
        remainder->connect_input(&shift_left_rem->get_outputs()[i], i);
    }
    remainder->connect_input(&dividend_bits[dividend_bit_index], 0);  // New bit at LSB
    remainder->connect_input(&write_enable->get_outputs()[0], num_bits);
    remainder->update();
    
    // Step 2: Try subtracting divisor from remainder
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        subtractor->connect_input(&remainder->get_outputs()[i], i);
        subtractor->connect_input(&divisor->get_outputs()[i], num_bits + i);
    }
    // Gated output enable - always evaluate but may not use result
    subtractor->connect_input(&write_enable->get_outputs()[0], 2 * num_bits + 1);
    subtractor->evaluate();
    
    // Step 3: Check carry flag (no borrow means remainder >= divisor)
    const bool* internal_output = subtractor->get_internal_output();
    
    bool quotient_bit;
    if (internal_output[num_bits])  // No borrow: remainder >= divisor (check carry directly)
    {
        // Update remainder with subtraction result
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            remainder->connect_input(&subtractor->get_outputs()[i], i);
        }
        remainder->connect_input(&write_enable->get_outputs()[0], num_bits);
        remainder->update();
        
        quotient_bit = true;
    }
    else  // Borrow occurred: remainder < divisor
    {
        // Keep old remainder (already in register, do nothing)
        quotient_bit = false;
    }
    
    // Step 4: Shift quotient left and insert new bit
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        shift_left_quot->connect_input(&quotient->get_outputs()[i], i);
    }
    shift_left_quot->connect_input(&write_enable->get_outputs()[0], num_bits);
    shift_left_quot->evaluate();
    
    // Store shifted quotient with new bit in LSB
    for (uint16_t i = 1; i < num_bits; ++i)
    {
        quotient->connect_input(&shift_left_quot->get_outputs()[i], i);
    }
    if (quotient_bit)
    {
        quotient->connect_input(&one_signal->get_outputs()[0], 0);
    }
    else
    {
        quotient->connect_input(&zero_signal->get_outputs()[0], 0);
    }
    quotient->connect_input(&write_enable->get_outputs()[0], num_bits);
    quotient->update();
    
    // Step 5: Check if done
    cycle_count++;
    if (cycle_count >= num_bits)
    {
        // Clear busy flag
        busy_flag->connect_input(&zero_signal->get_outputs()[0], 0);  // Set (inactive)
        busy_flag->connect_input(&one_signal->get_outputs()[0], 1);   // Reset (active)
        busy_flag->update();
    }
    
    // Evaluate AND gates and copy outputs
    for (uint16_t i = 0; i < 2 * num_bits; ++i)
    {
        output_AND_gates[i]->evaluate();
    }
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        outputs[i] = output_AND_gates[i]->get_output(0);
        outputs[num_bits + i] = output_AND_gates[num_bits + i]->get_output(0);
    }
    
    write_enable->go_low();
    } // End of if (is_busy())
    else
    {
        // Not busy - still evaluate AND gates for output gating
        for (uint16_t i = 0; i < 2 * num_bits; ++i)
        {
            output_AND_gates[i]->evaluate();
        }
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = output_AND_gates[i]->get_output(0);
            outputs[num_bits + i] = output_AND_gates[num_bits + i]->get_output(0);
        }
    }
}

void Divider_Sequential::update()
{
    evaluate();
    
    for (auto& component : downstream_components)
    {
        component->update();
    }
}
