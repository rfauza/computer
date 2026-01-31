#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <vector>
#include "../components/Component.hpp"
#include "../device_components/Flip_Flop.hpp"
#include "../device_components/Memory_Bit.hpp"

class Adder_Subtractor;


/**
 * @brief Tests a component with all combinations of input bits in truth table order
 * 
 * Creates signal generators for each input of the component, iterates through all
 * 2^num_inputs combinations (optionally starting from a given index), sets the inputs accordingly, and prints the results.
 * 
 * @param component Pointer to the component to test (uses component->get_num_inputs())
 * @param start_index Starting combination index (default 0). Useful to skip ahead to a specific test case.
 */
void test_truth_table(Component* component, int start_index = 0);

/**
 * @brief Tests Adder_Subtractor outputs against expected arithmetic results
 * 
 * Assumes input order: A bits, B bits, subtract enable, output enable (LSB first).
 * Computes expected outputs and reports mismatches.
 * 
 * @param device Reference to the Adder_Subtractor to test
 * @param start_index Starting combination index (default 0)
 */
void test_adder_subtractor(Adder_Subtractor& device, int start_index = 0);

/**
 * @brief Tests a Flip_Flop component with specific state transitions
 * 
 * Tests the flip-flop through multiple Set/Reset combinations to verify
 * state mem