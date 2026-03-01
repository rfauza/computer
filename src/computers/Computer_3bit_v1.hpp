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
 *   100 CMP   - Compare Out: set flags from [A] vs [B:C]  (B = page, C = addr)
 *   101 JEQ   - Jump if EQ:  goto [A:B:C]
 *   110 JGT   - Jump if GT:  goto [A:B:C]
 *   111 NOP   - No operation (options: CMP-in, MOVA-out, MOVA-in, rampage)
 *
 * Architecture: 3-bit data, 6-bit RAM addressing ([page:addr]),
 * 9-bit PC (512 PM addresses).
 */
class Computer_3bit_v1 : public Computer
{
public:
    /**
     * @brief Construct a Computer_3bit_v1 instance.
     *
     * Initializes the 3-bit ISA v1 computer and allocates core components.
     * The constructor delegates ISA-specific wiring to private helpers that
     * set up program memory <-> CPU connections, RAM addressing and
     * multiplexing, data paths and jump logic.
     *
     * @param name Optional name suffix used to create per-component names.
     */
    Computer_3bit_v1(const std::string& name = "");
    ~Computer_3bit_v1() override;

protected:
    /**
     * @brief Return human-readable mnemonic for an opcode.
     *
     * Maps numeric opcode values for ISA v1 to short mnemonic strings
     * (e.g. 0b010 -> "ADD"). Used by diagnostics and printing helpers.
     */
    std::string get_opcode_name(uint16_t opcode) const override;

private:
    static constexpr uint16_t NUM_BITS = 3;
    static constexpr uint16_t NUM_RAM_ADDR_BITS = 6;
    static constexpr uint16_t PC_BITS = 9;

    static const std::string ISA_V1_OPCODES;
    
    bool* cu_decoder;  ///< Pointer to CPU decoder outputs for opcode-based control signals
    Inverter* movl_not; ///< Inverter for MOVL control used by RAM data mux

    // constructor helper functions
    /**
     * @brief Wire program memory outputs into the CPU decoder and PM control.
     *
     * Sets up opcode wiring from PM to CPU, links CPU-driven PM address
     * inputs (program counter) and creates a small `Decoder` that produces
     * one-hot opcode signals directly from PM outputs for low-latency
     * observation by other components.
     */
    void _connect_program_memory_to_CPU_decoder();

    /**
     * @brief Orchestrate RAM-related wiring.
     *
     * Calls the address wiring, write-phase gating and data-multiplexing
     * helpers to fully prepare RAM inputs and control signals for this ISA.
     */
    void _connect_ram_inputs();

    /**
     * @brief Configure RAM address inputs and CMP-aware multiplexers.
     *
     * Wires read-port and write-port address inputs. Uses a PM-level
     * decoder output to control multiplexers that select between the
     * default [0:B] layout and the CMP-specific [B:C] layout for read
     * port 2.
     */
    void _connect_ram_address_inputs();

    /**
     * @brief Create CMP-controlled multiplexers for read-port-2 addressing.
     *
     * Allocates two `Multiplexer` instances (low and high address bits) that
     * select between [0:B] layout (ADD/SUB) and [B:C] layout (CMP) based on
     * the CMP decoder output. Also inverts the CMP signal to control the mux.
     */
    void _setup_ram_read_muxes();

    /**
     * @brief Wire per-bit RAM addresses and write-port controls.
     *
     * Connects read port 1 and 2 address lines, write address bits (C field
     * for low, gated B field for high via MOVL), and configures write-enable
     * OR gate for MOVL, ADD, and SUB instructions.
     */
    void _wire_ram_address_and_write_controls();

    /**
     * @brief Set up two-phase RAM read/write gating signals.
     *
     * Prevents read/write contention by creating a read-phase signal and
     * gating write enables so writes occur only during the write phase.
     */
    void _phase_ram_write_enable();

    /**
     * @brief Create and wire the RAM write-data multiplexer.
     *
     * Selects between ALU result and program-memory literal (MOVL) for
     * RAM write-data inputs using a `Multiplexer` device.
     */
    void _multiplex_RAM_data_inputs();

    /**
     * @brief Wire RAM data outputs into CPU data inputs.
     *
     * Collects pointers to RAM read-port outputs and the PM literal field
     * and passes them to the CPU as operand sources.
     */
    void _connect_ram_outputs();

    /**
     * @brief Assemble jump target address and configure conditional jumps.
     *
     * Concatenates A, B and C instruction fields into a PC-width jump
     * address and registers conditional jump conditions with the CPU.
     */
    void _connect_jump_logic();
};
