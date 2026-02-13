#pragma once
#include "Part.hpp"
#include "../devices/Register.hpp"
#include "../devices/Adder.hpp"
#include "../devices/Decoder.hpp"
#include "../components/Signal_Generator.hpp"
#include "../components/AND_Gate.hpp"
#include "../components/OR_Gate.hpp"
#include "../components/Inverter.hpp"
#include "../device_components/Flip_Flop.hpp"

/**
 * @brief Base Control Unit for managing program execution
 * 
 * This is a parent class that provides core control unit functionality.
 * Child classes will define specific opcodes and connections for different
 * computer architectures.
 * 
 * Features:
 * - Program Counter (PC): 2*num_bits register
 * - PC Incrementer: Adder for auto-increment
 * - PC Write Control: Selects between increment or external jump address
 * - Opcode Decoder: Decodes instructions to control signals
 * - Comparator Flags: Stores comparison results with auto-clear after 1 cycle
 * - RAM Page Register: 2*num_bits register for RAM paging
 * - Stack Pointer: 2*num_bits register for function calls
 * - Return Address Register: 2*num_bits register for function returns
 * 
 * child classes should wire decoder outputs to ALU enables and internal controls.
 */
class Control_Unit : public Part
{
public:
    /**
     * @brief Constructs a Control Unit with specified bit width
     * 
     * @param num_bits Base bit width (PC will be 2*num_bits)
     */
    Control_Unit(uint16_t num_bits, const std::string& name = "");
    
    virtual ~Control_Unit();
    
    /**
     * @brief Updates the control unit and propagates to downstream
     */
    void update() override;
    
    /**
     * @brief Evaluates internal components
     */
    void evaluate() override;
    
    // === Connection Methods for Different Subsystems ===
    
    /**
     * @brief Connect PC output to Program Memory address input
     * 
     * @param pm_address_input Pointer array to PM address inputs
     * @param start_index Index to start connecting PC bits
     * @return true if successful
     */
    bool connect_pc_to_pm_address(bool** pm_address_input, uint16_t start_index = 0);
    
    /**
     * @brief Connect external jump address to PC
     * 
     * @param jump_address_output Pointer array to jump address source
     * @param num_address_bits Number of address bits (should be 2*num_bits)
     * @return true if successful
     */
    bool connect_jump_address_to_pc(const bool* const* jump_address_output, uint16_t num_address_bits);
    
    /**
     * @brief Connect jump enable signal
     * 
     * @param jump_enable_signal Pointer to jump enable signal
     * @return true if successful
     */
    bool connect_jump_enable(const bool* jump_enable_signal);
    
    /**
     * @brief Connect opcode from Program Memory
     * 
     * @param opcode_output Pointer array to opcode bits
     * @param opcode_bits Number of opcode bits
     * @return true if successful
     */
    bool connect_opcode_input(const bool* const* opcode_output, uint16_t opcode_bits);
    
    /**
     * @brief Connect comparator flags from ALU
     * 
     * @param flag_outputs Pointer array to flag outputs (EQ, NEQ, LT_U, GT_U, LT_S, GT_S)
     * @param num_flags Number of flag signals (typically 6)
     * @return true if successful
     */
    bool connect_comparator_flags(const bool* const* flag_outputs, uint16_t num_flags);
    
    /**
     * @brief Connect RAM page register data input
     * 
     * @param page_data_input Pointer array to page data bits
     * @param num_page_bits Number of page bits (should be 2*num_bits)
     * @return true if successful
     */
    bool connect_ram_page_data_input(const bool* const* page_data_input, uint16_t num_page_bits);
    
    /**
     * @brief Connect RAM page write enable
     * 
     * @param page_write_enable Pointer to write enable signal
     * @return true if successful
     */
    bool connect_ram_page_write_enable(const bool* page_write_enable);
    
    /**
     * @brief Get pointer to PC outputs
     * 
     * @return Pointer to PC output array (2*num_bits)
     */
    bool* get_pc_outputs() const;
    
    /**
     * @brief Get pointer to decoder outputs
     * 
     * @return Pointer to decoder output array (2^opcode_bits)
     */
    bool* get_decoder_outputs() const;
    
    /**
     * @brief Get pointer to stored comparator flags
     * 
     * @return Pointer to flag register outputs
     */
    bool* get_stored_flags() const;
    
    /**
     * @brief Get pointer to RAM page register outputs
     * 
     * @return Pointer to RAM page outputs (2*num_bits)
     */
    bool* get_ram_page_outputs() const;
    
    /**
     * @brief Get pointer to stack pointer outputs
     * 
     * @return Pointer to stack pointer outputs (2*num_bits)
     */
    //bool* get_stack_pointer_outputs() const;  // DISABLED: inputs not connected
    
    /**
     * @brief Trigger a clock cycle (for flag clearing and other sequential logic)
     */
    void clock_tick();
    
protected:
    // === Core Components ===
    
    Register* pc;                     /**< Program Counter (2*num_bits) */
    Adder* pc_incrementer;            /**< Increments PC by 1 */
    Signal_Generator** increment_signals; /**< Array of signals for constant 1 (LSB high, rest low) */
    
    // PC Write Control
    Inverter* jump_enable_inverter;   /**< Inverts jump enable */
    AND_Gate** pc_increment_and_gates;    /**< Gates increment value with !jump_enable */
    AND_Gate** pc_jump_and_gates;         /**< Gates jump address with jump_enable */
    OR_Gate** pc_write_mux;           /**< Muxes increment vs jump for PC write */
    Signal_Generator* pc_write_enable; /**< Always high - PC always writes */
    Signal_Generator* pc_read_enable;  /**< Always high - PC always reads */
    
    // Opcode Decoder
    Decoder* opcode_decoder;          /**< Decodes opcode to control signals */
    uint16_t opcode_bits;             /**< Number of opcode bits */
    
    // Comparator Flags Storage with Auto-Clear
    Register* flag_register;          /**< Stores comparator flags */
    Signal_Generator* flag_write_enable; /**< Controls when flags are written */
    Signal_Generator* flag_read_enable;  /**< Always high - flags always read */
    Flip_Flop* flag_clear_counter;    /**< Flip-flop for 1-cycle delay */
    Signal_Generator* clear_set;      /**< Used to set the clear counter */
    Inverter* clear_inverter;         /**< Inverts clear signal to clear flags */
    uint16_t num_flags;               /**< Number of comparator flags */
    
    // RAM Page Register
    Register* ram_page_register;      /**< RAM paging register (2*num_bits) */
    Signal_Generator* ram_page_read_enable; /**< Always high - always read */
    
    // Default signals for optional external connections
    Signal_Generator* default_low_signal; /**< Default low signal for unconnected optional inputs */
    
    // Stack Management (for function calls) - DISABLED: inputs not connected
    //Register* stack_pointer;          /**< Stack pointer (2*num_bits) */
    //Register* return_address;         /**< Return address storage (2*num_bits) */
    //Signal_Generator* sp_read_enable; /**< Always high - SP always read */
    //Signal_Generator* ra_read_enable; /**< Always high - RA always read */
    
    uint16_t pc_bits;                 /**< PC bit width (2*num_bits) */
};

