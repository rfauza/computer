#pragma once
#include "Device.hpp"
#include "Adder_Subtractor.hpp"
#include "../components/Inverter.hpp"
#include "../components/XOR_Gate.hpp"
#include "../components/AND_Gate.hpp"
#include "../components/Signal_Generator.hpp"

/**
 * @brief Comparator that computes A-B and produces comparison results
 * 
 * Takes A and B inputs, internally computes A-B using an Adder_Subtractor,
 * and decodes the flags into 6 comparison outputs.
 * 
 * Input layout (2*num_bits):
 *   - inputs[0..num_bits-1]: A
 *   - inputs[num_bits..2*num_bits-1]: B
 * 
 * Output layout (6 comparison results):
 *   - outputs[0]: EQ (A == B)
 *   - outputs[1]: NEQ (A != B)
 *   - outputs[2]: LT_U (A < B unsigned)
 *   - outputs[3]: GT_U (A > B unsigned)
 *   - outputs[4]: LT_S (A < B signed)
 *   - outputs[5]: GT_S (A > B signed)
 */
class Comparator : public Device
{
public:
    Comparator(uint16_t num_bits);
    ~Comparator() override;
    void evaluate() override;
    void update() override;
    
private:
    Adder_Subtractor subtractor;  // Computes A-B for comparison
    Signal_Generator* always_high;  // Always high for subtract and output enable
    Inverter* not_z;
    Inverter* not_c;
    XOR_Gate* n_xor_v;
    Inverter* not_n_xor_v;
    AND_Gate* gt_u_and;
    AND_Gate* gt_s_and;
};
