#include "evaluator.hpp"
#include "isa_registry.hpp"
#include "../computers/Computer_3bit_v1.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>

// ── Internal helpers ──────────────────────────────────────────────────────────

std::string Evaluator::to_binary(uint16_t value, uint16_t bits) const
{
    std::string result;
    result.reserve(bits);
    for (int i = static_cast<int>(bits) - 1; i >= 0; --i)
        result += ((value >> i) & 1) ? '1' : '0';
    return result;
}

// ── Computer factory ──────────────────────────────────────────────────────────

static Computer* create_computer(const std::string& isa_key)
{
    if (isa_key == "3bit_v1")
        return new Computer_3bit_v1("eval_computer");

    std::cerr << "[evaluator] unsupported ISA: " << isa_key << "\n";
    return nullptr;
}

// ── .mc file parser ───────────────────────────────────────────────────────────

bool Evaluator::parse_mc_file(const std::string& path,
                               std::string& isa_key_out,
                               std::string& filename_out,
                               std::vector<MCInstruction>& instrs_out)
{
    std::ifstream f(path);
    if (!f.is_open())
    {
        std::cerr << "[evaluator] cannot open: " << path << "\n";
        return false;
    }

    isa_key_out.clear();
    filename_out.clear();
    instrs_out.clear();

    int instr_index = 0;
    std::string line;

    while (std::getline(f, line))
    {
        // Strip CR
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();

        if (line.empty())
            continue;

        if (line[0] == '#')
        {
            // Try to extract metadata
            std::string content = line.substr(1);
            size_t start = content.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            content = content.substr(start);

            auto colon = content.find(':');
            if (colon != std::string::npos)
            {
                std::string key   = content.substr(0, colon);
                std::string value = content.substr(colon + 1);
                while (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
                    key.pop_back();
                size_t vs = value.find_first_not_of(" \t");
                if (vs != std::string::npos) value = value.substr(vs);

                std::string key_lower = key;
                std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(),
                               [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

                if (key_lower == "isa")      { isa_key_out  = value; continue; }
                if (key_lower == "filename") { filename_out = value; continue; }
            }
            continue;
        }

        // Instruction line: binary_opc  binary_A  binary_B  binary_C  [# comment]
        std::string instr_part = line;
        std::string comment_text;
        auto hash = line.find('#');
        if (hash != std::string::npos)
        {
            instr_part    = line.substr(0, hash);
            comment_text  = line.substr(hash + 1);
            // Strip leading/trailing whitespace from comment
            size_t cs = comment_text.find_first_not_of(" \t");
            if (cs != std::string::npos) comment_text = comment_text.substr(cs);
        }

        std::istringstream iss(instr_part);
        std::string opc_s, a_s, b_s, c_s;
        if (!(iss >> opc_s >> a_s >> b_s >> c_s))
            continue; // skip malformed lines

        // Parse binary strings
        auto parse_bin = [](const std::string& s) -> uint16_t {
            uint16_t v = 0;
            for (char ch : s)
                v = static_cast<uint16_t>((v << 1) | (ch == '1' ? 1 : 0));
            return v;
        };

        MCInstruction mi;
        mi.line_num = instr_index++;
        mi.opcode   = parse_bin(opc_s);
        mi.a        = parse_bin(a_s);
        mi.b        = parse_bin(b_s);
        mi.c        = parse_bin(c_s);
        mi.comment  = comment_text;
        instrs_out.push_back(mi);
    }

    if (isa_key_out.empty())
    {
        std::cerr << "[evaluator] no '# isa:' header found in " << path << "\n";
        return false;
    }

    return true;
}

// ── Software simulation ───────────────────────────────────────────────────────

/**
 * Simulate one instruction against the given SimState (ISA 3bit_v1) and
 * populate `changed` with every RAM address that was modified.
 *
 * Architecture constants for 3bit_v1:
 *   num_bits = 3, page unit = 8, mask = 0x7
 *   RAM is flat 64-entry array: full_addr = page * 8 + addr
 */
void Evaluator::sim_step(const MCInstruction& instr,
                          const std::string& isa_key,
                          SimState& state,
                          std::vector<uint16_t>& changed,
                          uint16_t& new_pc) const
{
    changed.clear();

    // Currently only 3bit_v1 is implemented
    const uint16_t MASK = 0x7u; // 3-bit mask

    if (isa_key == "3bit_v1")
    {
        const uint16_t opc = instr.opcode;
        const uint16_t a   = instr.a;
        const uint16_t b   = instr.b;
        const uint16_t c   = instr.c;

        new_pc = static_cast<uint16_t>((state.pc + 1) & 0x1FFu); // default: PC+1

        switch (opc)
        {
            case 0b000: // HALT
                state.is_halted = true;
                // Hardware: the halt signal gates the PC incrementer to 0, so
                // PC resets to 0 after the HALT cycle.
                new_pc = 0;
                break;

            case 0b001: // MOVL  A → [B:C]  (literal a into RAM page b, addr c)
            {
                uint16_t addr = static_cast<uint16_t>((b << 3) | c);
                state.ram[addr] = a & MASK;
                changed.push_back(addr);
                break;
            }

            case 0b010: // ADD   [0:A] + [0:B] → [0:C]
            {
                uint16_t result = static_cast<uint16_t>((state.ram[a] + state.ram[b]) & MASK);
                state.ram[c] = result;
                changed.push_back(c);
                break;
            }

            case 0b011: // SUB   [0:A] - [0:B] → [0:C]
            {
                uint16_t result = static_cast<uint16_t>((state.ram[a] - state.ram[b] + 8u) & MASK);
                state.ram[c] = result;
                changed.push_back(c);
                break;
            }

            case 0b100: // CMP   flags ← [0:A] vs [B:C]
            {
                uint16_t addr2 = static_cast<uint16_t>((b << 3) | c);
                uint16_t val1  = state.ram[a];
                uint16_t val2  = state.ram[addr2];
                state.eq_flag   = (val1 == val2);
                state.gt_u_flag = (val1 >  val2);
                // no RAM change
                break;
            }

            case 0b101: // JEQ   if EQ: PC ← A:B:C
                if (state.eq_flag)
                    new_pc = static_cast<uint16_t>((a << 6) | (b << 3) | c);
                break;

            case 0b110: // JGT   if GT: PC ← A:B:C
                if (state.gt_u_flag)
                    new_pc = static_cast<uint16_t>((a << 6) | (b << 3) | c);
                break;

            case 0b111: // MOVOUT [rampage:A] → [B:C]
            {
                uint16_t src  = static_cast<uint16_t>((state.rampage << 3) | a);
                uint16_t dest = static_cast<uint16_t>((b << 3) | c);
                state.ram[dest] = state.ram[src];
                changed.push_back(dest);
                break;
            }

            default:
                std::cerr << "[evaluator] unknown opcode: " << opc << "\n";
                break;
        }
    }
    else
    {
        // Unknown ISA — produce no changes
        new_pc = static_cast<uint16_t>(state.pc + 1);
        std::cerr << "[evaluator] sim_step: unimplemented ISA '" << isa_key << "'\n";
    }

    state.pc = new_pc;
}

// ── Main evaluate() entry point ───────────────────────────────────────────────

bool Evaluator::evaluate(const std::string& mc_file, bool verbose)
{
    summary_ = Summary{};

    // ── Parse .mc file ────────────────────────────────────────────────────────
    std::string isa_key, filename;
    std::vector<MCInstruction> instructions;

    if (!parse_mc_file(mc_file, isa_key, filename, instructions))
        return false;

    // Normalize ISA key to lowercase
    std::transform(isa_key.begin(), isa_key.end(), isa_key.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    const ISA_Def* isa = get_isa(isa_key);
    if (!isa)
    {
        std::cerr << "[evaluator] unknown ISA: " << isa_key << "\n";
        return false;
    }

    std::cout << "\n"
              << std::string(60, '=') << "\n"
              << "Evaluating: " << filename << " (" << isa->display_name << ")\n"
              << std::string(60, '=') << "\n";

    // ── Build opcode name lookup ──────────────────────────────────────────────
    std::vector<std::string> opcode_names(1u << isa->num_bits, "???");
    for (const auto& op : isa->opcodes)
        if (op.opcode < opcode_names.size())
            opcode_names[op.opcode] = op.name;

    // ── Instantiate computer and load program ─────────────────────────────────
    Computer* computer = create_computer(isa_key);
    if (!computer)
        return false;

    if (!computer->load_program(mc_file))
    {
        delete computer;
        return false;
    }

    // ── Initialize software simulator state ───────────────────────────────────
    SimState sim;
    sim.pc      = 0;
    sim.rampage = 0;
    const uint16_t num_ram_addrs = static_cast<uint16_t>(1u << isa->num_ram_addr_bits);
    sim.ram.assign(static_cast<size_t>(num_ram_addrs), 0u);

    // Sync computer PC (make sure PM decoder reflects PC=0)
    computer->sync_pc();

    // ── Step-by-step execution and verification ───────────────────────────────
    std::cout << "\n";

    int cycle = 0;
    bool all_ok = true;
    // Safety: allow many cycles (programs with loops will execute multiple times)
    int max_cycles = std::max(1000, static_cast<int>(instructions.size()) * 100);

    while (!sim.is_halted && cycle < max_cycles)
    {
        uint16_t expected_pc = sim.pc;

        // Bounds check: PC must point into the loaded instruction list
        if (expected_pc >= static_cast<uint16_t>(instructions.size()))
        {
            std::cerr << "[evaluator] PC=" << expected_pc
                      << " exceeds program length (" << instructions.size()
                      << " instructions); stopping.\n";
            all_ok = false;
            break;
        }

        // Verify that the computer is about to execute the correct instruction
        uint16_t actual_pre_pc = computer->get_pc();
        bool pc_match = (actual_pre_pc == expected_pc);

        // Retrieve the instruction the computer currently has loaded
        const MCInstruction& instr = instructions[expected_pc];

        // Run the software simulation for this instruction
        std::vector<uint16_t> changed_addrs;
        uint16_t next_pc = 0;
        sim_step(instr, isa_key, sim, changed_addrs, next_pc);

        // Execute one hardware clock cycle
        computer->clock_tick();
        computer->sync_pc();

        uint16_t actual_post_pc = computer->get_pc();

        // ── Accumulate pass/fail results ──────────────────────────────────────
        bool step_ok = true;
        std::vector<std::string> step_failures;

        // Check PC was correct before the tick
        if (!pc_match)
        {
            step_ok = false;
            std::ostringstream oss;
            oss << "  PC mismatch before tick: expected=" << expected_pc
                << " actual=" << actual_pre_pc;
            step_failures.push_back(oss.str());
        }

        // Check post-tick PC (what the computer will execute next)
        if (actual_post_pc != next_pc)
        {
            step_ok = false;
            std::ostringstream oss;
            oss << "  PC mismatch after tick:  expected=" << next_pc
                << " actual=" << actual_post_pc;
            step_failures.push_back(oss.str());
        }

        // Check all RAM addresses (catches spurious writes too)
        uint16_t num_addrs = static_cast<uint16_t>(1u << isa->num_ram_addr_bits);
        for (uint16_t addr = 0; addr < num_addrs; ++addr)
        {
            uint16_t expected_val = sim.ram[addr];
            uint16_t actual_val   = computer->read_ram(addr);
            if (actual_val != expected_val)
            {
                step_ok = false;
                std::ostringstream oss;
                oss << "  RAM[" << std::setw(2) << addr << "] "
                    << "(page " << (addr >> isa->num_bits)
                    << " addr " << (addr &  ((1u << isa->num_bits) - 1)) << ")"
                    << ": expected=" << expected_val
                    << " actual=" << actual_val;
                step_failures.push_back(oss.str());
            }
        }

        // ── Status string for this instruction ────────────────────────────────
        const std::string& mnemonic = (instr.opcode < opcode_names.size())
                                    ? opcode_names[instr.opcode]
                                    : "???";

        if (step_ok)
        {
            ++summary_.passed;
            if (verbose)
            {
                std::cout << "[PASS] "
                          << std::setw(3) << cycle << " (PC=" << std::setw(3) << expected_pc << ")"
                          << " | " << mnemonic
                          << " " << instr.a
                          << " " << instr.b
                          << " " << instr.c;
                if (!changed_addrs.empty())
                {
                    std::cout << " | ";
                    for (uint16_t addr : changed_addrs)
                        std::cout << "RAM[" << addr << "]=" << sim.ram[addr] << " ";
                }
                else if (instr.opcode == 0b100) // CMP
                {
                    std::cout << " | EQ=" << sim.eq_flag
                              << " GT=" << sim.gt_u_flag;
                }
                std::cout << "\n";
            }
        }
        else
        {
            ++summary_.failed;
            all_ok = false;

            std::ostringstream fail_hdr;
            fail_hdr << "Step " << cycle << " (PC=" << expected_pc << ")"
                     << " " << mnemonic
                     << " " << instr.a << " " << instr.b << " " << instr.c;
            summary_.failures.push_back(fail_hdr.str());
            for (const auto& f : step_failures)
                summary_.failures.push_back(f);

            std::cout << "[FAIL] "
                      << std::setw(3) << cycle << " (PC=" << std::setw(3) << expected_pc << ")"
                      << " | " << mnemonic
                      << " " << instr.a << " " << instr.b << " " << instr.c << "\n";
            for (const auto& f : step_failures)
                std::cout << f << "\n";
        }

        ++cycle;
        summary_.total = cycle;
    }

    // ── Summary ───────────────────────────────────────────────────────────────
    std::cout << "\n" << std::string(60, '-') << "\n";
    std::cout << "Result: " << summary_.passed << "/" << summary_.total
              << " steps passed";
    if (summary_.failed > 0)
        std::cout << ", " << summary_.failed << " FAILED";
    std::cout << "\n";

    if (!summary_.failures.empty())
    {
        std::cout << "\nFailures:\n";
        for (const auto& f : summary_.failures)
            std::cout << "  " << f << "\n";
    }

    std::cout << std::string(60, '=') << "\n\n";

    delete computer;
    return all_ok;
}
