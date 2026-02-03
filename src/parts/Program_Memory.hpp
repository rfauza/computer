#pragma once
#include "Part.hpp"
#include "../devices/Decoder.hpp"
#include "../devices/Register.hpp"
#include "../devices/Bus.hpp"
#include "../components/AND_Gate.hpp"
#include <vector>

/**
 * @brief Program memory built from registers and a decoder
 * 
 * Addressed by num_bits selector inputs. Each address stores four registers:
 *   - opcode, C, A, B (each width = num_bits)
 * 
 * Input layout (5*num_bits + 2 total):
 *   - inputs[0..num_bits-1]           : address bits (LSB at index 0)
 *   - inputs[num_bits..5*num_bits-1]  : input bus (opcode, C, A, B)
 *   - inputs[5*num_bits]              : write_enable (WE)
 *   - inputs[5*num_bits + 1]          : read_enable (RE)
 * 
 * Output layout (4*num_bits total):
 *   - outputs[0..num_bits-1]              : opcode bus
 *   - outputs[num_bits..2*num_bits-1]     : C bus
 *   - outputs[2*num_bits..3*num_bits-1]   : A bus
 *   - outputs[3*num_bits..4*num_bits-1]   : B bus
 */
class Program_Memory : public Part
{
public:
    Program_Memory(uint16_t num_bits);
    ~Program_Memory() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;

private:
    static constexpr uint16_t registers_per_address = 4;

    uint16_t num_addresses = 0;
    uint16_t data_bus_bits = 0;

    Decoder decoder;
    Bus input_bus;
    Bus output_bus;

    Register** registers[4];   // num_addresses-sized array of 4-register arrays  (opcode, C, A, B)
    AND_Gate** write_selects;  // array of select gates (one per address)
    AND_Gate** read_selects;   // array of select gates (one per address)
};
