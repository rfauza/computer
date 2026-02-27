#pragma once

#include "Device.hpp"
#include "../components/AND_Gate.hpp"
#include "../components/OR_Gate.hpp"

class Multiplexer : public Device
{
public:
    // Constructor: Creates a num_bits-wide multiplexer with num_sources inputs
    // num_sources: number of data sources (any value: 2, 3, 4, ...)
    Multiplexer(uint16_t num_bits, uint16_t num_sources, const std::string& name = "");
    ~Multiplexer() override;

    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;

    // Connect data sources and their control signals
    // sources: array of data source pointers (num_sources arrays, each with num_bits pointers)
    // control_sigs: array of control signal pointers (num_sources signals)
    // For each bit: output[bit] = OR(source[0][bit] AND control[0], source[1][bit] AND control[1], ...)
    void connect_sources(const bool* const* const* sources, const bool* const* control_sigs);
    // Convenience: connect sources provided as arrays of bool values (one array per source)
    // Example: const bool* sources[] = { cpu_result, program_memory->get_outputs() };
    void connect_sources_from_values(const bool* const* sources_values, const bool* const* control_sigs);

private:
    uint16_t num_sources;
    
    // Control logic gates: for each source and each bit
    AND_Gate*** source_and_gates;  // source_and_gates[source][bit] = AND(source[bit], control_sig)
    OR_Gate** or_gates;            // or_gates[bit] = OR of all selected sources for that bit
};

