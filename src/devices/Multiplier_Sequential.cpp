#include "Multiplier_Sequential.hpp"
#include <sstream>
#include <iomanip>

Multiplier_Sequential::Multiplier_Sequential(uint16_t num_bits) : Device(num_bits), cycle_count(0)
{
    std::ostringstream oss;
    oss << "Multiplier_Sequential 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    
    num_inputs = 2 * num_bits + 2;  // A (num_bits) + B (num_bits) + start + output_enable
    num_outputs = 2 * num_bits + 1; // Product (2*num_bits) + busy
    
    allocate_IO_arrays();

    // Initialize outputs to false to avoid uninitialized values
    for (uint16_t i = 0; i < num_outputs; ++i)
    {
        outputs[i] = false;
    }
    
    // Create registers
    accumulator = new Register(2 * num_bits);
    multiplicand = new Register(2 * num_bits);  // Extended to 2*num_bits for shifting
    multiplier_reg = new Register(num_bits);
    busy_flag = new Flip_Flop();
    
    // Create combinational components
    adder = new Adder(2 * num_bits);
    shift_left = new L_Shift(2 * num_bits);
    shift_right = new R_Shift(num_bits);
    
    // Create control signals
    write_enable = new Signal_Generator();
    read_enable = new Signal_Generator();
    zero_signal = new Signal_Generator();
    one_signal = new Signal_Generator();
    
    write_enable->go_low();
    read_enable->go_high();  // Registers always readable
    zero_signal->go_low();
    one_signal->go_high();
    
    // Initialize output_enable pointer
    output_enable = nullptr;
    
    // Allocate AND gates for output gating
    output_AND_gates = new AND_Gate[2 * num_bits];
    
    // Connect accumulator outputs to AND gate input 0
    for (uint16_t i = 0; i < 2 * num_bits; ++i)
    {
        output_AND_gates[i].connect_input(&accumulator->get_outputs()[i], 0);
    }
    
    // Connect read enables (always high for combinational access)
    accumulator->connect_input(&read_enable->get_outputs()[0], 2 * num_bits + 1);
    multiplicand->connect_input(&read_enable->get_outputs()[0], 2 * num_bits + 1);
    multiplier_reg->connect_input(&read_enable->get_outputs()[0], num_bits + 1);
    
    // Wire outputs - busy flag gets direct pointer
    outputs[2 * num_bits] = &busy_flag->get_outputs()[0];  // Busy flag output
}

Multiplier_Sequential::~Multiplier_Sequential()
{
    delete[] output_AND_gates;
    delete accumulator;
    delete multiplicand;
    delete multiplier_reg;
    delete busy_flag;
    delete adder;
    delete shift_left;
    delete shift_right;
    delete write_enable;
    delete read_enable;
    delete zero_signal;
    delete one_signal;
}

bool Multiplier_Sequential::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // A inputs: 0 to num_bits-1
    // B inputs: num_bits to 2*num_bits-1
    // Start signal: 2*num_bits
    // Output enable: 2*num_bits+1
    
    if (input_index == 2 * num_bits + 1)
    {
        // Output enable: route to all AND gate input 1
        output_enable = inputs[input_index];
        bool result = true;
        for (uint16_t i = 0; i < 2 * num_bits; ++i)
        {
            result &= output_AND_gates[i].connect_input(output_enable, 1);
        }
        return result;
    }
    
    return true;
}

void Multiplier_Sequential::start()
{
    write_enable->go_high();
    
    // Load A into lower bits of multiplicand (upper bits are 0)
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        multiplicand->connect_input(inputs[i], i);
    }
    for (uint16_t i = num_bits; i < 2 * num_bits; ++i)
    {
        multiplicand->connect_input(&zero_signal->get_outputs()[0], i);
    }
    multiplicand->connect_input(&write_enable->get_outputs()[0], 2 * num_bits);
    multiplicand->update();
    
    // Load B into multiplier
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        multiplier_reg->connect_input(inputs[num_bits + i], i);
    }
    multiplier_reg->connect_input(&write_enable->get_outputs()[0], num_bits);
    multiplier_reg->update();
    
    // Clear accumulator
    for (uint16_t i = 0; i < 2 * num_bits; ++i)
    {
        accumulator->connect_input(&zero_signal->get_outputs()[0], i);
    }
    accumulator->connect_input(&write_enable->get_outputs()[0], 2 * num_bits);
    accumulator->update();
    
    // Set busy flag
    busy_flag->connect_input(&one_signal->get_outputs()[0], 0);  // Set
    busy_flag->connect_input(&zero_signal->get_outputs()[0], 1);  // Reset (inactive)
    busy_flag->update();
    
    write_enable->go_low();
    cycle_count = 0;
}

bool Multiplier_Sequential::is_busy() const
{
    return busy_flag->get_outputs()[0];
}

void Multiplier_Sequential::evaluate()
{
    if (is_busy())
    {
    
    write_enable->go_high();
    
    // Check LSB of multiplier
    bool lsb = multiplier_reg->get_outputs()[0];
    
    if (lsb)
    {
        // Add multiplicand to accumulator
        for (uint16_t i = 0; i < 2 * num_bits; ++i)
        {
            adder->connect_input(&accumulator->get_outputs()[i], i);
            adder->connect_input(&multiplicand->get_outputs()[i], 2 * num_bits + i);
        }
        adder->evaluate();
        
        // Store result back to accumulator
        for (uint16_t i = 0; i < 2 * num_bits; ++i)
        {
            accumulator->connect_input(&adder->get_outputs()[i], i);
        }
        accumulator->connect_input(&write_enable->get_outputs()[0], 2 * num_bits);
        accumulator->update();
    }
    
    // Shift multiplicand left
    for (uint16_t i = 0; i < 2 * num_bits; ++i)
    {
        shift_left->connect_input(&multiplicand->get_outputs()[i], i);
    }
    shift_left->connect_input(&write_enable->get_outputs()[0], 2 * num_bits);  // Enable
    shift_left->evaluate();
    
    for (uint16_t i = 0; i < 2 * num_bits; ++i)
    {
        multiplicand->connect_input(&shift_left->get_outputs()[i], i);
    }
    multiplicand->connect_input(&write_enable->get_outputs()[0], 2 * num_bits);
    multiplicand->update();
    
    // Shift multiplier right
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        shift_right->connect_input(&multiplier_reg->get_outputs()[i], i);
    }
    shift_right->connect_input(&write_enable->get_outputs()[0], num_bits);  // Enable
    shift_right->evaluate();
    
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        multiplier_reg->connect_input(&shift_right->get_outputs()[i], i);
    }
    multiplier_reg->connect_input(&write_enable->get_outputs()[0], num_bits);
    multiplier_reg->update();
    
    // Increment counter and check if done
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
        output_AND_gates[i].evaluate();
        outputs[i] = output_AND_gates[i].get_output(0);
    }
    
    write_enable->go_low();
    } // End of if (is_busy())
    else
    {
        // Not busy - still evaluate AND gates for output gating
        for (uint16_t i = 0; i < 2 * num_bits; ++i)
        {
            output_AND_gates[i].evaluate();
            outputs[i] = output_AND_gates[i].get_output(0);
        }
    }
}

void Multiplier_Sequential::update()
{
    evaluate();
    
    for (auto& component : downstream_components)
    {
        component->update();
    }
}
