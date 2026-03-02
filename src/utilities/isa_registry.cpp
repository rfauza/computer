#include "isa_registry.hpp"
#include <algorithm>
#include <cctype>

// ── Internal registry ─────────────────────────────────────────────────────────

static const std::vector<ISA_Def> s_isas = {
    {
        /* key          */ "3bit_v1",
        /* display_name */ "3-bit v1",
        /* num_bits     */ 3,
        /* ram_addr_bits*/ 6,
        /* pc_bits      */ 9,
        /* opcodes      */ {
            { "HALT",   0b000, false, "Stop execution" },
            { "MOVL",   0b001, false, "Move literal: A -> [B:C]" },
            { "ADD",    0b010, false, "Add:      [A] + [B] -> [C]   (page 0 addresses)" },
            { "SUB",    0b011, false, "Subtract: [A] - [B] -> [C]   (page 0 addresses)" },
            { "CMP",    0b100, false, "Compare:  flags <- [0:A] vs [B:C]" },
            { "JEQ",    0b101, true,  "Jump if equal:   PC <- A:B:C" },
            { "JGT",    0b110, true,  "Jump if greater: PC <- A:B:C" },
            { "MOVOUT", 0b111, false, "Move out: [rampage:A] -> [B:C]" },
        }
    },
};

// ── Public API ────────────────────────────────────────────────────────────────

const ISA_Def* get_isa(const std::string& key)
{
    // Normalise to lowercase for case-insensitive lookup
    std::string lower = key;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    for (const auto& isa : s_isas)
    {
        std::string isa_lower = isa.key;
        std::transform(isa_lower.begin(), isa_lower.end(), isa_lower.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (isa_lower == lower)
            return &isa;
    }
    return nullptr;
}

std::vector<std::string> list_isas()
{
    std::vector<std::string> keys;
    keys.reserve(s_isas.size());
    for (const auto& isa : s_isas)
        keys.push_back(isa.key);
    return keys;
}
