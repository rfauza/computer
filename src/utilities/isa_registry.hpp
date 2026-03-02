#pragma once
#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief Definition of a single ISA instruction.
 */
struct OpDef
{
    std::string name;         ///< Uppercase mnemonic (e.g. "ADD")
    uint16_t    opcode;       ///< Numeric opcode value (e.g. 0b010 for ADD)
    bool        is_jump;      ///< True if operand is a label/address split into A:B:C
    std::string description;  ///< Human-readable description for .mc header comments
};

/**
 * @brief Full definition of an ISA variant.
 *
 * Architecture parameters and the opcode table for one computer ISA.
 * Used by the assembler to convert mnemonics and by the evaluator to
 * instantiate the correct computer and run the software simulator.
 */
struct ISA_Def
{
    std::string         key;               ///< Lookup key (e.g. "3bit_v1")
    std::string         display_name;      ///< Pretty name (e.g. "3-bit v1")
    uint16_t            num_bits;          ///< Data-path width in bits
    uint16_t            num_ram_addr_bits; ///< Total RAM address bits
    uint16_t            pc_bits;           ///< Program counter width in bits
    std::vector<OpDef>  opcodes;           ///< Opcode table (opcode value order)
};

/**
 * @brief Look up an ISA definition by its key string.
 *
 * @param key  ISA key (case-insensitive, e.g. "3bit_v1").
 * @return Pointer to the ISA_Def, or nullptr if not found.
 */
const ISA_Def* get_isa(const std::string& key);

/**
 * @brief List all registered ISA keys.
 */
std::vector<std::string> list_isas();
