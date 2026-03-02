#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

/**
 * @brief Assembles a .ass (assembly) file into a .mc (machine code) file.
 *
 * Assembly file format (.ass):
 * ─────────────────────────────
 *   # comment lines (any line beginning with #)
 *   # filename: <name>        (optional — inferred from file path if omitted)
 *   # isa: <isa_key>          (required — e.g.  3bit_v1)
 *   # description: <text>     (optional free-form description)
 *
 *   def <label>               (marks the next instruction's address as <label>)
 *   OPNAME arg1 arg2 arg3     (instruction with up to 3 numeric arguments)
 *   OPNAME <label>            (jump instruction: label resolves to A:B:C address)
 *
 * Machine code file format (.mc):
 * ────────────────────────────────
 *   # <description>
 *   # filename: <name>
 *   # isa: <isa_key>
 *   # <extra header comment lines copied from .ass>
 *   # <ISA opcode table>
 *   # generated_from: <input_file>
 *   opc  A   B   C   # <line_num> - <assembly text>
 *   ...
 *
 * Label resolution for jump instructions:
 *   A label value `addr` is split into three fields of `num_bits` width each:
 *     A = addr >> (2 * num_bits),  B = (addr >> num_bits) & mask,  C = addr & mask
 */
class Assembler
{
public:
    /**
     * @brief Construct an Assembler.
     *
     * No ISA is loaded at construction time; the ISA is read from the .ass
     * file header.
     */
    Assembler() = default;

    /**
     * @brief Assemble a .ass file and write the result to a .mc file.
     *
     * @param input_file   Path to the source assembly file (*.ass).
     * @param output_file  Path to the destination machine-code file (*.mc).
     *                     If empty, derived from input_file by replacing the
     *                     extension with ".mc".
     * @return true on success, false if any error occurred.
     */
    bool assemble(const std::string& input_file,
                  const std::string& output_file = "");

    /** @brief Return the list of errors encountered during the last assemble(). */
    const std::vector<std::string>& errors() const { return errors_; }

private:
    struct ParsedInstruction
    {
        int         source_line;   ///< 1-based line number in the .ass file
        std::string mnemonic;      ///< Upper-cased mnemonic
        std::string raw_args[3];   ///< Raw argument strings (numeric or label)
        int         num_args;      ///< Number of args provided (0-3)
        bool        is_label_jump; ///< True when a single label arg was given
        std::string label_arg;     ///< The label name (when is_label_jump)
        std::string comment;       ///< Optional inline comment
    };

    std::vector<std::string> errors_;

    void emit_error(int source_line, const std::string& msg);
    std::string to_binary(uint16_t value, uint16_t bits) const;
    std::string derive_output_path(const std::string& input_file) const;
    std::string derive_filename(const std::string& path) const;
    std::string to_upper(const std::string& s) const;
};
