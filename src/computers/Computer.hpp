#pragma once
#include "../parts/Part.hpp"
#include "../parts/CPU.hpp"
#include "../parts/Program_Memory.hpp"
#include "../parts/Main_Memory.hpp"
#include "../devices/Decoder.hpp"
#include "../devices/Multiplexer.hpp"
#include "../devices/Register.hpp"
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
 * evaluate, toggle_ram_read_flag, to_binary, from_binary.
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

    /** @brief Reset the program counter to address 0 and clear halt state. */
    void reset_pc();

    /** @brief Set the program counter to a specific address. */
    void set_pc(uint16_t address);

    /** @brief Erase all RAM (write zeros to every address). */
    void reset_ram();

    /**
     * @brief Full reset: zero all RAM, zero all PM, set PC to 0, clear halt.
     * Leaves the machine ready to program from scratch.
     */
    void reset_all();

    /** @brief Clear the halt state so execution can resume from the current PC. */
    void clear_halt();

    void evaluate() override;

    // ── State query helpers (used by Evaluator) ───────────────────────────────

    /** @brief Return the current program counter value. */
    uint16_t get_pc() const;

    /**
     * @brief Re-evaluate the Program Memory decoder so get_pc() reflects
     *        the CPU's current PC register outputs.
     *
     * Call this after clock_tick() when you need an up-to-date PC value.
     * (The PM decoder is normally updated only at the start of each
     * clock_tick(); calling sync_pc() between ticks keeps it in sync.)
     */
    void sync_pc();

    /**
     * @brief Read a RAM register value by its full address.
     * @param address Full RAM address (0 to num_ram_addresses-1).
     * @return The stored value at that address.
     */
    uint16_t read_ram(uint16_t address) const;

    /** @brief Return data-path width in bits. */
    uint16_t get_num_bits() const { return num_bits; }

    /** @brief Return total number of RAM addresses. */
    uint16_t get_num_ram_addresses() const { return num_ram_addresses; }

    /** @brief Return the ISA version string set by the subclass. */
    const std::string& get_isa_version() const { return ISA_version; }

    /** @brief Return whether the computer is still running (not halted). */
    bool get_is_running() const { return is_running; }
    
    /** @brief Return the PC width in bits. */
    uint16_t get_pc_bits() const { return pc_bits; }
    
    /** @brief Return total number of PM addresses (2^pc_bits). */
    uint16_t get_num_pm_addresses() const { return num_pm_addresses; }
    
    /** @brief Return the number of RAM address bits. */
    uint16_t get_num_ram_addr_bits() const { return num_ram_address_bits; }
    
    // ── GUI support ──────────────────────────────────────────────────────────
    
    /**
     * @brief Read an instruction from Program Memory at the given address.
     *
     * Directly reads register contents without modifying any connections
     * or triggering evaluation.
     */
    void read_pm_instruction(uint16_t address, uint16_t& opcode,
                             uint16_t& a, uint16_t& b, uint16_t& c) const;
    
    /**
     * @brief Write an instruction to Program Memory at the given address.
     *
     * Uses internal signal generators to temporarily set the address and
     * data, pulse write-enable, then restores the PC connection.
     */
    void write_pm_instruction(uint16_t address, uint16_t opcode,
                              uint16_t a, uint16_t b, uint16_t c);
    
    /**
     * @brief Read the current instruction from PM outputs.
     *
     * Reads the outputs array of program_memory which reflects the
     * instruction at the current PC address.
     */
    void get_current_instruction(uint16_t& opcode, uint16_t& a,
                                 uint16_t& b, uint16_t& c) const;
    
    /**
     * @brief Get comparator flags from the CPU.
     * @return Pointer to bool array [EQ, NEQ, LT_U, GT_U, LT_S, GT_S],
     *         or nullptr if CPU has no flags.
     */
    bool* get_cmp_flags() const;
    
    /**
     * @brief Prepare the computer for a fresh run after programming.
     *
     * Re-evaluates Program Memory so the first instruction is visible,
     * resets the running flag, and clears execution count.
     */
    void prepare_run();
    
    // ───End:  State query helpers (used by Evaluator) ───────────────────────────────────

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

    // ── Page registers (numbits in size, initialized to all 0's) ──────────────
    Register*        rampage;              ///< RAM page selector for ALU operations
    Register*        opcodepage;           ///< Extension bits for CPU decoder

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
    Multiplexer* ram_data_mux;

    // ── Write-address high-bits mux (gates B field with MOVL enable) ─────────
    AND_Gate**  ram_write_addr_high_mux;
    
    // ── PM opcode decoder (decodes opcode bits before the CPU evaluates) ──────
    // Created by the subclass after program_memory is wired. Provides one-hot
    // outputs so subclasses can reference any opcode directly without manual
    // AND/NOT logic. Defaults to nullptr; subclass initialises it.
    Decoder*    pm_decoder;               ///< one-hot decoder for PM opcode bits

    // ── Read-port-2 address mux (used by CMP-style instructions) ─────────────
    // Multiplexes RAM read-port-2 address between the normal [0:B] layout and
    // the CMP-specific [B:C] layout. Both default to nullptr; wired by the
    // subclass inside _connect_ram_address_inputs() when it needs CMP support.
    Inverter*    cmp_not;                  ///< NOT(CMP enable signal)
    Multiplexer* ram_read2_addr_mux_low;   ///< selects B vs C for read-port-2 low address bits
    Multiplexer* ram_read2_addr_mux_high;  ///< selects 0 vs B for read-port-2 high address bits

    // ── PM loading signal generators (kept alive to avoid use-after-free) ─────
    std::vector<Signal_Generator>*          pm_load_addr_sigs;
    std::vector<Signal_Generator>*          pm_load_data_sigs;
    std::vector<Signal_Generator>*          pm_zero_sigs;   ///< Permanent zeros to restore PM data inputs after writes
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
    uint16_t from_binary(const std::string& binary) const;

    /// Returns the human-readable mnemonic for the given opcode value.
    /// Must be overridden by each ISA subclass.
    virtual std::string get_opcode_name(uint16_t opcode) const = 0;

    /// Create a unique component name string with memory address and optional name.
    void _create_namestring(const std::string& name);
    
    /// Print architecture details using computer_version and ISA_version.
    void _print_architecture_details() const;

    /**
     * @brief Evaluate ISA-specific logic gates (called during phase 2 before
     * write-address gates). Subclasses may override to evaluate gates like
     * instruction-decode OR gates that control write addressing.
     * Default implementation is a no-op.
     */
    virtual void evaluate_isa_write_gates();
};
