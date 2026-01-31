#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <vector>
#include <iostream>
#include <string>
#include "../components/Component.hpp"
#include "../device_components/Flip_Flop.hpp"
#include "../device_components/Memory_Bit.hpp"

class Adder_Subtractor;

/**
 * @brief Tests a component with a specific binary input string
 * 
 * Creates signal generators for each input, sets them according to the binary string,
 * updates the device, and prints the output.
 * 
 * @param device Pointer to the component to test
 * @param binary_input Binary string where each character is '0' or '1' (LSB first)
 */
void test_component(Component* device, const std::string& binary_input);

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

/**
 * @brief Tests a component type with a range of input sizes
 * 
 * Template function that instantiates a component of the given type with
 * input sizes from min_inputs to max_inputs, and runs test_truth_table() on each instance.
 * 
 * @tparam T The component type to test (e.g., AND_Gate, OR_Gate, XOR_Gate)
 * @param min_inputs Minimum number of inputs to test (default 1)
 * @param max_inputs Maximum number of inputs to test (default 4)
 */
template<typename T>
void test_component_type(uint16_t min_inputs = 1, uint16_t max_inputs = 4)
{
    for (uint16_t inputs = min_inputs; inputs <= max_inputs; ++inputs)
    {
        Component* comp = new T(inputs);
        std::cout << "\n=== " << comp->get_component_name() << " with " << inputs << " input(s) ===" << std::endl;
        test_truth_table(comp);
        delete comp;
    }
}

/**
 * @brief Tests all logic component types with a range of input sizes
 * 
 * Calls test_component_type() for each standard logic component type:
 * AND_Gate, OR_Gate, NAND_Gate, NOR_Gate, XOR_Gate, Buffer, and Inverter.
 * Each component is tested with input sizes from min_inputs to max_inputs.
 * 
 * @param min_inputs Minimum number of inputs to test (default 1)
 * @param max_inputs Maximum number of inputs to test (default 4)
 */
void test_all_components(uint16_t min_inputs = 1, uint16_t max_inputs = 4);

#endif

