#pragma once
#include "Part.hpp"
#include "../devices/Decoder.hpp"
#include "../devices/Register.hpp"
#include "../components/AND_Gate.hpp"
#include <vector>

/**
 * @brief Main memory built from registers and a decoder
 * 
 * Addressed by address_bits selector inputs. Each address stores one register.
 * 
 * Input layout (address_bits + data_bits + 2 total):
 *   - inputs[0..address_bits-1]                    : address bits (LSB at index 0)
 *   - inputs[address_bits..address_bits+data_bits-1] : input data bus
 *   - inputs[address_bits + data_bits]              : write_enable (WE)
 *   - inputs[address_bits + data_bits + 1]          : read_enable (RE)
 * 
 * Output layout (data_bits total):
 *   - outputs[0..data_bits-1]    : output data bus
 */
class Main_Memory : public Part
{
public:
    Main_Memory(uint16_t address_bits = 8, uint16_t data_bits = 4, const std::string& name = "");
    ~Main_Memory() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    void update() override;
    
    uint16_t get_address_bits() const { return address_bits; }
    uint16_t get_data_bits() const { return data_bits; }

private:
    uint16_t address_bits = 0;
    uint16_t num_addresses = 0;
    uint16_t data_bits = 0;

    Decoder decoder;
    AND_Gate** write_selects;
    AND_Gate** read_selects;
    Register** registers;  // Array of Register pointers
};

