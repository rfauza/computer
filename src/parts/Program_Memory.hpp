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
 * Addressed by decoder_bits selector inputs. Each address stores 4 registers:
 *   - opcode, C, A, B (each width = data_bits)
 * 
 * Input layout (decoder_bits + 4*data_bits + 2 total):
 *   - inputs[0..decoder_bits-1]                      : address bits (LSB at index 0)
 *   - inputs[decoder_bits..decoder_bits+4*data_bits-1] : input bus (opcode, C, A, B)
 *   - inputs[decoder_bits + 4*data_bits]            : write_enable (WE)
 *   - inputs[decoder_bits + 4*data_bits + 1]        : read_enable (RE)
 * 
 * Output layout (4*data_bits total):
 *   - outputs[0..data_bits-1]              : opcode bus
 *   - outputs[data_bits..2*data_bits-1]    : C bus
 *   - outputs[2*data_bits..3*data_bits-1]  : A bus
 *   - outputs[3*data_bits..4*data_bits-1]  : B bus
 */
class Program_Memory : public Part
{
public:
    Program_Memory(uint16_t decoder_bits = 12, uint16_t data_bits = 4);
    ~Program_Memory() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;
    
    uint16_t get_decoder_bits() const { return decoder_bits; }
    uint16_t get_data_bits() const { return data_bits; }

private:
    static constexpr uint16_t registers_per_address = 4;

    uint16_t decoder_bits = 0;
    uint16_t num_addresses = 0;
    uint16_t data_bits = 0;

    Decoder decoder;
    AND_Gate** write_selects;
    AND_Gate** read_selects;
    Register** registers[4]; // 4 arrays of Register* (opcode, C, A, B)
};
