#include "assembler.hpp"
#include "isa_registry.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <filesystem>

// ── Internal helpers ──────────────────────────────────────────────────────────

void Assembler::emit_error(int source_line, const std::string& msg)
{
    std::ostringstream oss;
    if (source_line > 0)
        oss << "line " << source_line << ": ";
    oss << msg;
    errors_.push_back(oss.str());
    std::cerr << "[assembler] error: " << oss.str() << "\n";
}

std::string Assembler::to_binary(uint16_t value, uint16_t bits) const
{
    std::string result;
    result.reserve(bits);
    for (int i = static_cast<int>(bits) - 1; i >= 0; --i)
        result += ((value >> i) & 1) ? '1' : '0';
    return result;
}

std::string Assembler::derive_output_path(const std::string& input_file) const
{
    std::filesystem::path p(input_file);
    p.replace_extension(".mc");
    return p.string();
}

std::string Assembler::derive_filename(const std::string& path) const
{
    std::filesystem::path p(path);
    std::filesystem::path name = p.filename();
    // Strip repeated extensions (e.g. "cursor.ass.mc" -> "cursor")
    while (name.has_extension())
        name = name.stem();
    return name.string();
}

std::string Assembler::to_upper(const std::string& s) const
{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
    return result;
}

// ── assemble() ────────────────────────────────────────────────────────────────

bool Assembler::assemble(const std::string& input_file,
                         const std::string& output_file_arg)
{
    errors_.clear();

    // ── Open input ────────────────────────────────────────────────────────────
    std::ifstream in(input_file);
    if (!in.is_open())
    {
        emit_error(0, "cannot open input file: " + input_file);
        return false;
    }

    // ── Parse the source ──────────────────────────────────────────────────────
    std::string isa_key;
    std::string filename_meta;
    std::string description_meta;

    std::map<std::string, int> label_map;            // label -> instruction index
    std::vector<ParsedInstruction> instructions;     // one per real instruction
    std::vector<std::string> raw_lines;              // raw assembly text per instruction (for output)

    int src_line_num = 0;
    int instr_index  = 0;
    std::string line;

    while (std::getline(in, line))
    {
        ++src_line_num;

        // Strip trailing whitespace / CR
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();

        if (line.empty())
            continue;

        // ── Comment / header line ─────────────────────────────────────────────
        if (line[0] == '#')
        {
            // Try to extract header metadata
            std::string content = line.substr(1);
            // Strip leading whitespace from content
            size_t start = content.find_first_not_of(" \t");
            if (start != std::string::npos)
                content = content.substr(start);

            // Check for key: value pairs
            auto colon = content.find(':');
            if (colon != std::string::npos)
            {
                std::string key_raw   = content.substr(0, colon);
                std::string value_raw = content.substr(colon + 1);
                // Strip whitespace
                while (!key_raw.empty() && (key_raw.back() == ' ' || key_raw.back() == '\t'))
                    key_raw.pop_back();
                size_t vs = value_raw.find_first_not_of(" \t");
                if (vs != std::string::npos)
                    value_raw = value_raw.substr(vs);

                std::string key_lower = key_raw;
                std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(),
                               [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

                if (key_lower == "isa")
                {
                    isa_key = value_raw;
                    continue;
                }
                if (key_lower == "filename")
                {
                    filename_meta = value_raw;
                    continue;
                }
                if (key_lower == "description")
                {
                    description_meta = value_raw;
                    continue;
                }
            }

            // Generic header comment — ignored; assembler will generate its own header
            continue;
        }

        // ── def <label> ───────────────────────────────────────────────────────
        {
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            std::string token_lower = token;
            std::transform(token_lower.begin(), token_lower.end(), token_lower.begin(),
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            if (token_lower == "def")
            {
                std::string label;
                if (!(iss >> label))
                {
                    emit_error(src_line_num, "def requires a label name");
                    continue;
                }
                if (label_map.count(label))
                {
                    emit_error(src_line_num, "duplicate label: " + label);
                    continue;
                }
                label_map[label] = instr_index;
                continue;
            }
        }

        // ── Instruction line ──────────────────────────────────────────────────
        // Strip inline comment first
        std::string instr_part = line;
        std::string inline_comment;
        auto hash_pos = line.find('#');
        if (hash_pos != std::string::npos)
        {
            instr_part = line.substr(0, hash_pos);
            inline_comment = line.substr(hash_pos); // keep the '#'
            while (!instr_part.empty() && (instr_part.back() == ' ' || instr_part.back() == '\t'))
                instr_part.pop_back();
        }

        std::istringstream iss(instr_part);
        std::string mnemonic;
        if (!(iss >> mnemonic))
            continue; // blank after stripping comment

        ParsedInstruction pi;
        pi.source_line   = src_line_num;
        pi.mnemonic      = to_upper(mnemonic);
        pi.num_args      = 0;
        pi.is_label_jump = false;
        pi.comment       = inline_comment;

        std::string tok;
        int arg_idx = 0;
        while (iss >> tok && arg_idx < 3)
        {
            pi.raw_args[arg_idx++] = tok;
        }
        pi.num_args = arg_idx;

        // Record the raw assembly text for the .mc comment
        std::ostringstream raw;
        raw << mnemonic;
        for (int i = 0; i < pi.num_args; ++i)
            raw << " " << pi.raw_args[i];
        raw_lines.push_back(raw.str());

        instructions.push_back(pi);
        ++instr_index;
    }
    in.close();

    // ── Validate ISA ──────────────────────────────────────────────────────────
    if (isa_key.empty())
    {
        emit_error(0, "no ISA specified (add '# isa: <name>' to the file header)");
        return false;
    }

    const ISA_Def* isa = get_isa(isa_key);
    if (!isa)
    {
        emit_error(0, "unknown ISA: '" + isa_key + "'");
        return false;
    }

    // Build opcode lookup: mnemonic -> opcode value
    std::map<std::string, const OpDef*> op_lookup;
    for (const auto& op : isa->opcodes)
        op_lookup[op.name] = &op;

    uint16_t mask    = static_cast<uint16_t>((1u << isa->num_bits) - 1);
    uint16_t pc_mask = static_cast<uint16_t>((1u << isa->pc_bits)  - 1);

    // ── Second pass: encode instructions ─────────────────────────────────────
    struct EncodedInstr
    {
        uint16_t opcode, a, b, c;
        std::string assembly_text;
    };
    std::vector<EncodedInstr> encoded;
    encoded.reserve(instructions.size());

    bool ok = true;
    for (int idx = 0; idx < static_cast<int>(instructions.size()); ++idx)
    {
        const ParsedInstruction& pi = instructions[idx];

        auto it = op_lookup.find(pi.mnemonic);
        if (it == op_lookup.end())
        {
            emit_error(pi.source_line, "unknown mnemonic: " + pi.mnemonic);
            ok = false;
            continue;
        }

        const OpDef* op = it->second;

        EncodedInstr ei;
        ei.opcode = op->opcode;
        ei.a = ei.b = ei.c = 0;
        ei.assembly_text = raw_lines[idx];

        if (op->is_jump)
        {
            // Single label arg: resolve to address and split into A:B:C
            if (pi.num_args == 1)
            {
                const std::string& label = pi.raw_args[0];
                // Check if it looks like a number (allow label OR numeric)
                bool is_numeric = !label.empty() &&
                    label.find_first_not_of("01") == std::string::npos;
                if (!is_numeric)
                {
                    // Treat as label
                    auto lit = label_map.find(label);
                    if (lit == label_map.end())
                    {
                        emit_error(pi.source_line, "undefined label: " + label);
                        ok = false;
                        continue;
                    }
                    uint16_t addr = static_cast<uint16_t>(lit->second) & pc_mask;
                    ei.a = static_cast<uint16_t>((addr >> (2 * isa->num_bits)) & mask);
                    ei.b = static_cast<uint16_t>((addr >>      isa->num_bits)  & mask);
                    ei.c = static_cast<uint16_t>( addr                         & mask);
                    // Annotate assembly text with resolved address
                    std::ostringstream ann;
                    ann << pi.mnemonic << " " << label << " (->" << lit->second << ")";
                    ei.assembly_text = ann.str();
                }
                else
                {
                    // Single binary literal treated as full address
                    uint16_t addr = 0;
                    for (char ch : label)
                        addr = static_cast<uint16_t>((addr << 1) | (ch == '1' ? 1 : 0));
                    addr &= pc_mask;
                    ei.a = static_cast<uint16_t>((addr >> (2 * isa->num_bits)) & mask);
                    ei.b = static_cast<uint16_t>((addr >>      isa->num_bits)  & mask);
                    ei.c = static_cast<uint16_t>( addr                         & mask);
                }
            }
            else if (pi.num_args == 3)
            {
                // Three explicit binary args
                auto parse_bin = [](const std::string& s) -> uint16_t {
                    uint16_t v = 0;
                    for (char c : s) v = static_cast<uint16_t>((v << 1) | (c == '1' ? 1 : 0));
                    return v;
                };
                ei.a = parse_bin(pi.raw_args[0]) & mask;
                ei.b = parse_bin(pi.raw_args[1]) & mask;
                ei.c = parse_bin(pi.raw_args[2]) & mask;
            }
            else
            {
                emit_error(pi.source_line,
                           pi.mnemonic + ": jump instruction needs 1 label or 3 address fields");
                ok = false;
                continue;
            }
        }
        else
        {
            // Normal instruction: parse up to 3 binary/decimal args
            auto parse_field = [&](const std::string& s, uint16_t& out, int field_idx) -> bool {
                if (s.empty())
                {
                    out = 0;
                    return true;
                }
                // Binary string?
                if (s.find_first_not_of("01") == std::string::npos)
                {
                    uint16_t v = 0;
                    for (char c : s) v = static_cast<uint16_t>((v << 1) | (c == '1' ? 1 : 0));
                    out = v & mask;
                    return true;
                }
                // Decimal?
                try
                {
                    long v = std::stol(s);
                    if (v < 0 || v > mask)
                    {
                        emit_error(pi.source_line,
                                   "argument " + std::to_string(field_idx) + " value " + s +
                                   " out of range (0-" + std::to_string(mask) + ")");
                        return false;
                    }
                    out = static_cast<uint16_t>(v);
                    return true;
                }
                catch (...)
                {
                    emit_error(pi.source_line, "cannot parse argument: " + s);
                    return false;
                }
            };

            bool arg_ok = true;
            if (!parse_field(pi.num_args > 0 ? pi.raw_args[0] : "", ei.a, 0)) arg_ok = false;
            if (!parse_field(pi.num_args > 1 ? pi.raw_args[1] : "", ei.b, 1)) arg_ok = false;
            if (!parse_field(pi.num_args > 2 ? pi.raw_args[2] : "", ei.c, 2)) arg_ok = false;
            if (!arg_ok)
            {
                ok = false;
                continue;
            }
        }

        encoded.push_back(ei);
    }

    if (!ok)
        return false;

    // ── Determine output path / filename metadata ─────────────────────────────
    std::string out_path = output_file_arg.empty()
                         ? derive_output_path(input_file)
                         : output_file_arg;
    // If the source provided a filename header, normalize it by stripping
    // any extensions (so "cursor.ass" -> "cursor"). Otherwise derive
    // the filename from the output path.
    std::string filename_base;
    if (filename_meta.empty())
        filename_base = derive_filename(out_path);
    else
        filename_base = derive_filename(filename_meta);
    // Always present the filename metadata with a .mc extension
    filename_meta = filename_base + ".mc";
    if (description_meta.empty())
        description_meta = filename_meta;

    // ── Write .mc file (write to temp then atomically replace existing file) ──
    std::filesystem::path target(out_path);
    std::filesystem::path temp_path = target;
    temp_path += ".tmp";

    // Remove any existing temp file
    std::error_code ec;
    if (std::filesystem::exists(temp_path, ec))
        std::filesystem::remove(temp_path, ec);

    std::ofstream out(temp_path);
    if (!out.is_open())
    {
        emit_error(0, "cannot write temporary output file: " + temp_path.string());
        return false;
    }

    // Header: generate our own comments (filename then ISA)
    out << "# filename: " << filename_meta << "\n";
    out << "# isa: " << isa->key << "\n";
    out << "# " << isa->display_name << " ISA:\n";
    for (const auto& op : isa->opcodes)
    {
        std::string padded_name = op.name;
        while (padded_name.size() < 8)
            padded_name += ' ';
        out << "#   " << to_binary(op.opcode, isa->num_bits)
            << "  " << padded_name
            << " - " << op.description << "\n";
    }
    out << "# generated_from: " << std::filesystem::path(input_file).filename().string() << "\n";
    out << "\n";

    // Instructions
    for (int i = 0; i < static_cast<int>(encoded.size()); ++i)
    {
        const EncodedInstr& ei = encoded[i];
        out << to_binary(ei.opcode, isa->num_bits) << " "
            << to_binary(ei.a,      isa->num_bits) << " "
            << to_binary(ei.b,      isa->num_bits) << " "
            << to_binary(ei.c,      isa->num_bits)
            << " # " << i << " - " << ei.assembly_text << "\n";
    }

    out.close();

    // Replace existing target file with the temp file
    if (std::filesystem::exists(target, ec))
    {
        if (!std::filesystem::remove(target, ec))
        {
            emit_error(0, "cannot remove existing output file: " + target.string());
            // attempt to clean temp then fail
            std::filesystem::remove(temp_path, ec);
            return false;
        }
    }

    std::filesystem::rename(temp_path, target, ec);
    if (ec)
    {
        emit_error(0, "cannot move temporary file to output path: " + ec.message());
        // attempt to clean temp
        std::filesystem::remove(temp_path, ec);
        return false;
    }

    std::cout << "[assembler] wrote " << encoded.size()
              << " instruction(s) to " << target.string() << "\n";
    return true;
}
