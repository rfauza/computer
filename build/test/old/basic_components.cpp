#include "basic_components.hpp"
#include <iostream>
using namespace std;

void Component::update()
{
    evaluate(); 
    
    for(uint16_t i=0; i<num_downstream_components; i++)
    {
        downstream_components[i]->update();
    }
}

void Component::print_outputs()
{
    cout << typeid(*this).name() << " " << this << " has " << (*this).num_outputs << " outputs" << endl;
    cout << " outputs: [";
    for(int i=0; i<this->num_outputs; i++)
    {
        cout << this->output[i];
        if (i < this->num_outputs -1) cout << ", ";
    }
    cout << "]" << endl;
}

Signal_Generator::Signal_Generator()
{
    ; // do nothing
}

void Signal_Generator::go_high()
{
    internal_output[0] = 1;
}

void Signal_Generator::go_low()
{
    internal_output[0] = 0;
}

bool Signal_Generator::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    cout << "This device (" << typeid(*this).name() << ") does not accept any inputs." << endl;
    return false;
}

bool Signal_Generator::connect_output(Component* const downstream_component_p, const bool* const this_output_p, uint16_t downstream_index)
{
    bool connection_was_successful = false;
    
    if (downstream_component_p != nullptr)
       {
            connection_was_successful = downstream_component_p->connect_input(this_output_p, downstream_index);
       }
    
    return connection_was_successful;
}


void Buffer::evaluate()
{
    for(int i=0; i<num_outputs; i++)
    {
        internal_output[i] = *input[0];
    }
    return;
}

bool Buffer::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    if ((upstream_output_p != nullptr) && (input_index >= 0) && (input_index < num_inputs))
    {
        cout << "buffer connecting " << upstream_output_p << " to index " << input_index << endl;
        input[input_index] = upstream_output_p;
    }
    
    return false;
}

bool Buffer::connect_output(Component* const downstream_component_p, const bool* const this_output_p, uint16_t downstream_index)
{
    bool connection_was_successful = false;
    
    if (downstream_component_p != nullptr)
       {
            connection_was_successful = downstream_component_p->connect_input(this_output_p, downstream_index);
       }
    
    return connection_was_successful;
}



// void And::evaluate()
// {
//     output[0] = *input[0] && *input[1];
// }

