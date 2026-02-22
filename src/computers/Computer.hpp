#pragma once
#include "../parts/Part.hpp"
#include "../parts/CPU.hpp"
#include "../parts/Program_Memory.hpp"
#include "../parts/Main_Memory.hpp"
#include "../components/Signal_Generator.hpp"
#include "../components/OR_Gate.hpp"
#include "../components/AND_Gate.hpp"
#include "../components/Inverter.hpp"
#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief Abstract base class for all computer variants.
 *
 * Holds all shared infrastructure (CPU, PM, RAM, control gates, signal
 * generators) and implements every method that is independent of the ISA:
 * load_program, run_interactive, clock_tick, print_state, reset,
 * evaluate, update, toggle_ram_read_flag, to_binary, from_binary.
 *
 * Subclasses must:
 *   1. Call Computer(num_bits, num_ram_address_bits, pc_bits, name) from
 *      their initializer list.
 *   2. Wire all ISA-specific connections in their constructor body
 *      (CPU opcode table, PM/RAM connections, jump conditions, mux logic,
 *      write-enable OR gate, read-flag gating, etc.).
 *   3. Override get_opcode_name() to return human-readable instruction names.
 */
class Computer : public Part
{
public:
    /**
     * @param num_bits           Data-path width in bits.
     * @param num_ram_addr_bits  Total RAM address bits (e.g. 6 for [page:addr]).
     * @param pc_bits            Program-counter width in bits.
     * @param name               Optional component name.
     */
    Computer(uint16_t num_bits, uint16_t num_ram_addr_bits,
             uint16_t pc_bits, const std::string& name = "");

    ~Computer() override;

    /**
     * @brief Load a program from a text file into Program Memory.
     *
     * File format — one instruction per line, whitespace-separated fields:
     *   opcode A B C   (binary or decimal)
     * Lines starting with '#' or blank lines are ignored.
     */
    bool load_program(const std::string& filename);

    /**
     * @brief Run the loaded program.
     *
     * When `interactive` is true (default) the runner prompts per cycle;
     * otherwise it executes continuously until a HALT instruction.
     */
    void run(bool interactive = true);

    /**
     * @brief Advance one clock cycle.
     * @return true while running, false once halted.
     */
    bool clock_tick();

    /**
     * @brief Print PC, current instruction, and RAM contents.
     */
    void print_state() const;

    /**
     * @brief Reset the computer to initial state (stub; TODO).
     */
    void reset();

    void evaluate() override;
    void update() override;

protected:
    // ── Runtime dimensions (set once by the constructor) ──────────────────────
    uint16_t num_bits;
    uint16_t num_ram_address_bits;
    uint16_t num_ram_addresses;
    uint16_t pc_bits;
    uint16_t num_pm_addresses;

    // ── Core components (created and wired by the subclass constructor) ───────
    CPU*             cpu;
    Program_Memory*  program_memory;
    Main_Memory*     ram;

    // ── Control signals ───────────────────────────────────────────────────────
    Signal_Generator* pm_write_enable;
    Signal_Generator* pm_read_enable;
    Signal_Generator* ram_write_enable;
    Signal_Generator* ram_read_enable;
    Signal_Generator* read_addr_high_low;   ///< Constant-low for unused address bits
    OR_Gate*          ram_write_or;         ///< OR of all write-enabling opcodes

    // ── Two-phase write gating ────────────────────────────────────────────────
    // ram_we_gated = NOT(ram_read_flag) AND ram_write_or
    Signal_Generator* ram_read_flag;        ///< HIGH during read phase
    Inverter*         ram_read_flag_not;
    AND_Gate*         ram_we_gated;

    // ── RAM data-input mux (MOVL literal vs. CPU result) ─────────────────────
    Inverter*   ram_data_mux_not;
    AND_Gate**  ram_data_mux_and_literal;
    AND_Gate**  ram_data_mux_and_result;
    OR_Gate**   ram_data_mux_or;

    // ── Write-address high-bits mux (gates B field with MOVL enable) ─────────
    AND_Gate**  ram_write_addr_high_mux;

    // ── PM loading signal generators (kept alive to avoid use-after-free) ─────
    std::vector<Signal_Generator>*          pm_load_addr_sigs;
    std::vector<Signal_Generator>*          pm_load_data_sigs;
    mutable std::vector<Signal_Generator>*  ram_addr_sigs;

    // ── CPU data-input pointer arrays (lifetime matches the Computer) ─────────
    const bool** data_a_ptrs;
    const bool** data_b_ptrs;
    const bool** data_c_ptrs;

    bool     is_running;
    uint64_t execution_count;

    // Human-readable version strings for the computer instance and ISA.
    // Subclasses or external code may set these to document the build/version
    // of the computer and the instruction set architecture in use.
    std::string computer_version;
    std::string ISA_version;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void toggle_ram_read_flag(bool flag_high);

    std::string to_binary(uint16_t value, uint16_t bits) const;
    uint16_t    from_binary(const std::string& binary) const;

    /// Returns the human-readable mnemonic for the given opcode value.
    /// Must be overridden by each ISA subclass.
    virtual std::string get_opcode_name(uint16_t opcode) const = 0;

    /// Create a unique component name string with memory address and optional name.
    void _create_namestring(const std::string& name);
    
    /// Print architecture details using computer_version and ISA_version.
    void _print_architecture_details() const;
};
