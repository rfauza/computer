#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

/**
 * @brief Instantiates a computer from a .mc file, runs the program
 *        step-by-step, and verifies correctness against a software simulation.
 *
 * The evaluator performs a parallel software simulation of the program.  After
 * each clock tick it compares the actual hardware-level RAM and PC against the
 * expected state produced by the software simulation.  Any mismatch is flagged
 * as a FAIL.
 *
 * Machine code file header requirements:
 *   # isa: <isa_key>        — selects the computer architecture
 *   # filename: <name>      — used for display purposes
 *
 * Supported ISAs: 3bit_v1
 */
class Evaluator
{
public:
    Evaluator() = default;

    /**
     * @brief Load, run, and evaluate a .mc program.
     *
     * @param mc_file  Path to the .mc machine-code file to evaluate.
     * @param verbose  If true, print all instruction results (not only FAILs).
     * @return true if every instruction passed verification, false otherwise.
     */
    bool evaluate(const std::string& mc_file, bool verbose = true);

    /** @brief Return the pass/fail summary from the last evaluate() call. */
    struct Summary
    {
        int passed  = 0;
        int failed  = 0;
        int total   = 0;
        std::vector<std::string> failures; ///< Human-readable failure descriptions
    };

    const Summary& summary() const { return summary_; }

private:
    Summary summary_;

    // ── Parsed instruction (from .mc file) ────────────────────────────────────
    struct MCInstruction
    {
        int      line_num;  ///< 0-based instruction index in program
        uint16_t opcode;
        uint16_t a, b, c;
        std::string comment; ///< Text after the '#' on that line
    };

    // ── Software simulator state ──────────────────────────────────────────────
    struct SimState
    {
        std::vector<uint16_t> ram;   ///< Full RAM contents (all pages flat)
        uint16_t              pc;
        uint16_t              rampage;
        bool eq_flag   = false;
        bool gt_u_flag = false;
        bool is_halted = false;
    };

    bool parse_mc_file(const std::string& path,
                       std::string& isa_key_out,
                       std::string& filename_out,
                       std::vector<MCInstruction>& instrs_out);

    /**
     * @brief Simulate one instruction against SimState and record which RAM
     *        addresses should have changed so the result can be verified.
     *
     * @param instr     The instruction to simulate.
     * @param isa_key   ISA identifier (lowercase) for ISA-specific semantics.
     * @param state     Current simulator state (modified in place).
     * @param changed   Addresses written to by this instruction.
     * @param new_pc    The PC value the computer should show after the tick.
     */
    void sim_step(const MCInstruction& instr,
                  const std::string& isa_key,
                  SimState& state,
                  std::vector<uint16_t>& changed,
                  uint16_t& new_pc) const;

    std::string to_binary(uint16_t value, uint16_t bits) const;
};
