#include "Component.hpp"
#include <iostream>

Component::~Component()
{
    if (inputs != nullptr)
        delete[] inputs;
    if (outputs != nullptr)
        delete[] outputs;
}

void Component::evaluate()
{
    // Default implementation does nothing
    // Leaf components override this for their logic
}

void Component::update()
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

void Component::print_outputs() const
{
    std::cout << component_name << " outputs: ";
    for (uint16_t i = 0; i < num_outputs; ++i)
    {
        std::cout << outputs[i];
        if (i < num_outputs - 1) std::cout << ", ";
    }
    std::cout << std::endl;
}

bool Component::get_output(uint16_t output_index) const
{
    if (output_index >= num_outputs)
    {
        return false;
    }
    return outputs[output_index];
}

bool* Component::get_outputs() const
{
    return outputs;
}

void Component::print_inputs() const
{
    std::cout << component_name << " inputs: ";
    for (uint16_t i = 0; i < num_inputs; ++i)
    {
        if (inputs[i])
        {
            std::cout << *inputs[i];
        }
        else
        {
            std::cout << "null";
        }
        if (i < num_inputs - 1) std::cout << ", ";
    }
    std::cout << std::endl;
}

void Component::print_io() const
{
    print_inputs();
    print_outputs();
}

void Component::set_component_name(const std::string& name)
{
    component_name = name;
}

void Component::initialize_IO_arrays()
{
    // Allocate input pointers
    if (num_inputs > 0)
    {
        inputs = new bool*[num_inputs];
        for (uint16_t i = 0; i < num_inputs; ++i)
        {
            inputs[i] = nullptr;
        }
    }
    else
    {
        inputs = nullptr;
    }
    
    // Allocate outputs
    if (num_outputs > 0)
    {
        outputs = new bool[num_outputs];
        for (uint16_t i = 0; i < num_outputs; ++i)
        {
            outputs[i] = false;
        }
    }
    else
    {
        outputs = nullptr;
    }
}

void Component::allocate_IO_arrays()
{
    // Allocate input pointers
    if (num_inputs > 0)
    {
        inputs = new bool*[num_inputs];
        for (uint16_t i = 0; i < num_inputs; ++i)
        {
            inputs[i] = nullptr;
        }
    }
    else
    {
        inputs = nullptr;
    }
    
    // Allocate outputs (without initializing values)
    if (num_outputs > 0)
    {
        outputs = new bool[num_outputs];
    }
    else
    {
        outputs = nullptr;
    }
}

bool Component::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    if (input_index >= num_inputs)
    {
        std::cerr << "Error: " << component_name << " - input index " << input_index 
                  << " out of range (max: " << num_inputs - 1 << ")" << std::endl;
        return false;
    }
    inputs[input_index] = const_cast<bool*>(upstream_output_p);
    return true;
}

bool Component::connect_output(Component* const downstream_component_p, uint16_t output_index, uint16_t downstream_index)
{
    if (!downstream_component_p)
    {
        std::cerr << "Error: " << component_name << " - downstream component pointer is null" << std::endl;
        return false;
    }
    
    if (output_index >= num_outputs)
    {
        std::cerr << "Error: " << component_name << " - output index " << output_index 
                  << " out of range (max: " << num_outputs - 1 << ")" << std::endl;
        return false;
    }
    
    // Add downstream component to list
    downstream_components.push_back(downstream_component_p);
    
    // Connect this output to the downstream component's input
    // Pass address of our output value
    return downstream_component_p->connect_input(&outputs[output_index], downstream_index);
}
