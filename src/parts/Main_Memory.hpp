#pragma once
#include "Part.hpp"
#include "../devices/Decoder.hpp"
#include "../devices/Register.hpp"
#include "../components/AND_Gate.hpp"
#include <vector>

/**
 * @brief Triple-ported Main Memory (2 read ports + 1 write port)
 * 
 * Supports simultaneous read from two addresses and write to a third in one cycle.
 * 
 * Input layout (3*address_bits + data_bits + 3 total):
 *   - inputs[0..address_bits-1]                             : address A (read port 1)
 *   - inputs[address_bits..2*address_bits-1]                : address B (read port 2)
 *   - inputs[2*address_bits..3*address_bits-1]              : address C (write port)
 *   - inputs[3*address_bits..3*address_bits+data_bits-1]    : write data
 *   - inputs[3*address_bits+data_bits]                      : write_enable (WE)
 *   - inputs[3*address_bits+data_bits+1]                    : read_enable_A (RE_A)
 *   - inputs[3*address_bits+data_bits+2]                    : read_enable_B (RE_B)
 * 
 * Output layout (2*data_bits total):
 *   - outputs[0..data_bits-1]         : data A output (from port A)
 *   - outputs[data_bits..2*data_bits-1] : data B output (from port B)
 */
class Main_Memory : public Part
{
public:
    Main_Memory(uint16_t address_bits = 8, uint16_t data_bits = 4, const std::string& name = "");
    ~Main_Memory() override;
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
    void evaluate() override;
    // void update() override;
    
    uint16_t get_address_bits() const { return address_bits; }
    uint16_t get_data_bits() const { return data_bits; }
    uint16_t get_num_addresses() const { return num_addresses; }
    
    /**
     * @brief Get the value stored in a register at the given address
     * @param address The address of the register (0 to num_addresses-1)
     * @return The value stored in that register as a uint16_t
     */
    uint16_t get_register_value(uint16_t address) const;
    
    /**
     * @brief Print RAM contents organized by pages in a grid format
     * @param num_pages Number of pages to print (4 pages per row). If more than available, prints up to what exists.
     */
    void print_pages(uint16_t num_pages) const;
    
    /**
     * @brief Debug: Print all register contents without modifying connections or evaluating
     */
    void print_all_registers() const;
    
    /**
     * @brief Debug: Print select gate outputs (write_selects, read_selects)
     * This is read-only and does not call evaluate().
     */
    void print_selects() const;

    /**
     * @brief Print grouped IO for debugging: A,B,C,data,flags
     * Format: A_bits, B_bits, C_bits, data_bits, WEREAREB (e.g. 100000,010000,011000,101,111)
     */
    void print_inputs() const override;
    void print_outputs() const override;

private:
    uint16_t address_bits = 0;
    uint16_t num_addresses = 0;
    uint16_t data_bits = 0;

    Decoder decoder_a;  // Decoder for read port A
    Decoder decoder_b;  // Decoder for read port B
    Decoder decoder_c;  // Decoder for write port C
    AND_Gate** write_selects;
    AND_Gate** read_selects_a;
    AND_Gate** read_selects_b;
    Register** registers;  // Array of Register pointers
};

