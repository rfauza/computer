#include "XOR_Gate.hpp"
#include <sstream>
#include <iomanip>

XOR_Gate::XOR_Gate(uint16_t num_inputs_param, const std::string& name)
    : Component(name)
{
    num_inputs = num_inputs_param;
    // create component name string (include hex address and optional provided name)
    std::ostringstream oss;
    oss << "XOR_Gate 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    num_outputs = 1;
    
    // Allocate inputs
    inputs = new bool*[num_inputs];
    for (uint16_t i = 0; i < num_inputs; ++i)
        inputs[i] = nullptr;
    
    // Allocate outputs as bool array
    outputs = new bool[num_outputs];
    
    // Create buffers and inverters for each input
    // Create one num_inputs-input AND gate per input
    // Each AND will be: input[i] AND NOT(all other inputs)
    for (uint16_t i = 0; i < num_inputs; ++i)
    {
        std::ostringstream buf_name;
        std::ostringstream inv_name;
        std::ostringstream and_name;
        if (!name.empty()) {
            buf_name << name << "_input_buffer_" << i;
            inv_name << name << "_input_inverter_" << i;
            and_name << name << "_and_" << i;
        } else {
            buf_name << "input_buffer_" << i << "_in_xor_gate";
            inv_name << "input_inverter_" << i << "_in_xor_gate";
            and_name << "and_" << i << "_in_xor_gate";
        }

        input_buffers.push_back(new Buffer(1, buf_name.str()));
        input_inverters.push_back(new Inverter(1, inv_name.str()));
        and_gates.push_back(new AND_Gate(num_inputs, and_name.str()));
    }
    
    // Create multi-input OR gate
    {
        std::ostringstream or_name;
        if (!name.empty()) or_name << name << "_output_or";
        else or_name << "output_or_in_xor_gate";
        output_or_gate = new OR_Gate(num_inputs, or_name.str());
    }
    
    // Connect each buffer output to its corresponding inverter
    for (uint16_t i = 0; i < num_inputs; ++i)
    {
        input_buffers[i]->connect_output(input_inverters[i], 0, 0);
    }
    
    // Wire each AND gate: input[i] & !input[0] & !input[1] & ... (excluding i)
    for (uint16_t i = 0; i < num_inputs; ++i)
    {
        uint16_t and_input_idx = 0;
        for (uint16_t j = 0; j < num_inputs; ++j)
        {
            if (i == j)
            {
                // Use the buffer (non-inverted input)
                input_buffers[j]->connect_output(and_gates[i], 0, and_input_idx);
            }
            else
            {
                // Use the inverter (inverted input)
                input_inverters[j]->connect_output(and_gates[i], 0, and_input_idx);
            }
            and_input_idx++;
        }
        
        // Connect this AND gate to the output OR
        and_gates[i]->connect_output(output_or_gate, 0, i);
    }
};

XOR_Gate::~XOR_Gate()
{
    for (auto buffer : input_buffers)
        delete buffer;
    for (auto inverter : input_inverters)
        delete inverter;
    for (auto and_gate : and_gates)
        delete and_gate;
    delete output_or_gate;
}

bool XOR_Gate::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    if (input_index >= num_inputs)
        return false;
    
    // Wire the appropriate buffer with the actual signal
    return input_buffers[input_index]->connect_input(inputs[input_index], 0);
}

void XOR_Gate::evaluate()
{
    // Evaluate all internal components in topological order
    // First evaluate all buffers
    for (auto buffer : input_buffers)
        buffer->evaluate();
    
    // Then evaluate all inverters
    for (auto inverter : input_inverters)
        inverter->evaluate();
    
    // Then evaluate all AND gates
    for (auto and_gate : and_gates)
        and_gate->evaluate();
    
    // Finally evaluate the output OR gate
    output_or_gate->evaluate();
    
    // Copy OR gate output to our output
    outputs[0] = output_or_gate->get_output(0);
}

void XOR_Gate::update()
{
    // Evaluate internal components
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

