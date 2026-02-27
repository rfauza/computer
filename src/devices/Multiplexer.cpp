#include "Multiplexer.hpp"
#include <sstream>
#include <iostream>

Multiplexer::Multiplexer(uint16_t num_bits_, uint16_t num_sources_, const std::string& name)
    : Device(num_bits_, name),
      num_sources(num_sources_),
      source_and_gates(nullptr),
      or_gates(nullptr)
{
    // Allocate source_and_gates[num_sources][num_bits]
    source_and_gates = new AND_Gate**[num_sources];
    for (uint16_t source = 0; source < num_sources; ++source)
    {
        source_and_gates[source] = new AND_Gate*[num_bits];
        for (uint16_t bit = 0; bit < num_bits; ++bit)
        {
            std::ostringstream gate_name;
            gate_name << name << "_source_" << source << "_bit_" << bit;
            source_and_gates[source][bit] = new AND_Gate(2, gate_name.str());
        }
    }

    // Allocate OR gates to combine all sources for each bit
    or_gates = new OR_Gate*[num_bits];
    for (uint16_t bit = 0; bit < num_bits; ++bit)
    {
        std::ostringstream or_name;
        or_name << name << "_or_" << bit;
        or_gates[bit] = new OR_Gate(num_sources, or_name.str());
    }
}

Multiplexer::~Multiplexer()
{
    // Delete source AND gates
    if (source_and_gates)
    {
        for (uint16_t source = 0; source < num_sources; ++source)
        {
            if (source_and_gates[source])
            {
                for (uint16_t bit = 0; bit < num_bits; ++bit)
                {
                    delete source_and_gates[source][bit];
                }
                delete[] source_and_gates[source];
            }
        }
        delete[] source_and_gates;
    }

    // Delete OR gates
    if (or_gates)
    {
        for (uint16_t bit = 0; bit < num_bits; ++bit)
        {
            delete or_gates[bit];
        }
        delete[] or_gates;
    }
}

void Multiplexer::connect_sources(const bool* const* const* sources, const bool* const* control_sigs)
{
    // For each source and each bit, connect the AND gate
    for (uint16_t source = 0; source < num_sources; ++source)
    {
        for (uint16_t bit = 0; bit < num_bits; ++bit)
        {
            // AND gate inputs: source data bit and control signal for this source
            source_and_gates[source][bit]->connect_input(sources[source][bit], 0);
            source_and_gates[source][bit]->connect_input(control_sigs[source], 1);
        }
    }

    // For each bit, connect all source AND outputs to the OR gate
    for (uint16_t bit = 0; bit < num_bits; ++bit)
    {
        for (uint16_t source = 0; source < num_sources; ++source)
        {
            or_gates[bit]->connect_input(&source_and_gates[source][bit]->get_outputs()[0], source);
        }
    }
}

bool Multiplexer::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Not implementing generic input connection for now;
    // use connect_sources() instead to set up the data sources.
    std::cerr << "Error: connect_input() not supported for Multiplexer '"
              << get_component_name() << "' (tried to connect input index "
              << input_index << "). Use connect_sources() instead." << std::endl;
              
    // Silence unused parameter warnings
    (void)upstream_output_p;
    (void)input_index;
    
    return false;
}

void Multiplexer::evaluate()
{
    // Evaluate AND gates
    for (uint16_t source = 0; source < num_sources; ++source)
    {
        for (uint16_t bit = 0; bit < num_bits; ++bit)
        {
            source_and_gates[source][bit]->evaluate();
        }
    }

    // Evaluate OR gates and copy outputs
    for (uint16_t bit = 0; bit < num_bits; ++bit)
    {
        or_gates[bit]->evaluate();
        outputs[bit] = or_gates[bit]->get_output(0);
    }
}
