#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <vector>
#include "../components/Component.hpp"
#include "../device_components/Flip_Flop.hpp"
#include "../device_components/Memory_Bit.hpp"


/**
 * @brief Tests a component with all combinations of input bits in truth table order
 * 
 * Creates signal generators for each input of the component, iterates through all
 * 2^num_inputs combinations, sets the inputs accordingly, and prints the results.
 * 
 * @param component Pointer to the component to test (uses component->get_num_inputs())
 */
void test_truth_table(Component* component);

/**
 * @brief Tests a Flip_Flop component with specific state transitions
 * 
 * Tests the flip-flop through multiple Set/Reset combinations to verify
 * state memory and transitions.
 * 
 * @param device Reference to the Flip_Flop to test
 */
void flip_flop_tester(Flip_Flop& device);

/**
 * @brief Tests a Memory_Bit component with a specific sequence of write/read operations
 * 
 * Tests the memory bit through a sequence of data writes and reads to verify
 * proper storage and retrieval.
 * 
 * @param device Reference to the Memory_Bit to test
 */
void memory_bit_tester(Memory_Bit& device);

#endif

