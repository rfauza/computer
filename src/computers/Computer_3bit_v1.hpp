#pragma once
#include "Computer.hpp"
#include <string>
#include <cstdint>

/**
 * @brief 3-bit Computer implementing ISA v1.
 *
 * ISA v1 opcodes:
 *   000 HALT  - Stop execution
 *   001 MOVL  - Move literal: A -> [B:C]
 *   010 ADD   - Add:      [A] + [B] -> [C]
 *   011 SUB   - Subtract: [A] - [B] -> [C]
 *   100 CMP   - Compare: set flags from [A] vs [B]
 *   101 JEQ   - Jump if EQ:  goto [A:B:C]
 *   110 JGT   - Jump if GT:  goto [A:B:C]
 *   111 NOP   - No operation
 *
 * Architecture: 3-bit data, 6-bit RAM addressing ([page:addr]),
 * 9-bit PC (512 PM addresses).
 */
class Computer_3bit_v1 : public Computer
{
public:
    Computer_3bit_v1(const std::string& name = "");
    ~Computer_3bit_v1() override = default;

protected:
    std::string get_opcode_name(uint16_t opcode) const override;

private:
    static constexpr uint16_t NUM_BITS            = 3;
    static constexpr uint16_t NUM_RAM_ADDR_BITS   = 6;
    static constexpr uint16_t PC_BITS             = 9;

    static const std::string ISA_V1_OPCODES;
};
