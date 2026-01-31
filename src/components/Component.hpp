#pragma once

#include <vector>
#include <cstdint>
#include <string>

/**
 * @brief Abstract base class for all logic components
 * 
 * This class provides the foundation for all circuit components, managing
 * inputs, outputs, and connections between components in the circuit.
 */
class Component
{
public:
    /**
     * @brief Default constructor
     */
    Component() = default;
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Component();

    /**
     * @brief Runs internal calculations and updates outputs based on current inputs
     * 
     * Virtual method that can be overridden by leaf components.
     * This is called by update() to perform the component's logic.
     */
    virtual void evaluate();
    
    /**
     * @brief Calls evaluate() and signals all downstream components to update themselves
     * 
     * This method propagates updates through the circuit, ensuring all connected
     * components are notified of changes.
     */
    virtual void update();
    
    /**
     * @brief Connects output of an upstream device to an input of this component
     * 
     * @param upstream_output_p Pointer to boolean from upstream component's output
     * @param input_index Index of this component's input to attach the connection to
     * @return true if connection was successful, false otherwise
     */
    virtual bool connect_input(const bool* const upstream_output_p, uint16_t input_index);
    
    /**
     * @brief Connects one output of this component to an input of a downstream component
     * 
     * @param downstream_component_p Pointer to component which will receive this output
     * @param output_index Index of this component's output to connect
     * @param downstream_index Index of the downstream component's input to connect to
     * @return true if connection was successful, false otherwise
     */
    virtual bool connect_output(Component* const downstream_component_p, uint16_t output_index, uint16_t downstream_index);
    
    /**
     * @brief Gets the value of a specific output
     * 
     * @param output_index Index of the output to retrieve
     * @return The boolean value of the output, or false if index out of range
     */
    bool get_output(uint16_t output_index) const;
    
    /**
     * @brief Gets pointer to the outputs array
     * 
     * @return Pointer to the boolean array of outputs
     */
    bool* get_outputs() const;
    
    /**
     * @brief Prints all outputs of this component to standard output
     */
    void print_outputs();
    
    /**
     * @brief Prints all inputs of this component to standard output
     */
    void print_inputs();
    
    /**
     * @brief Prints all inputs and outputs of this component to standard output
     */
    void print_io();
    
    /**
     * @brief Gets the number of inputs for this component
     * 
     * @return The number of inputs
     */
    uint16_t get_num_inputs() const { return num_inputs; }
    
    /**
     * @brief Gets the number of outputs for this component
     * 
     * @return The number of outputs
     */
    uint16_t get_num_outputs() const { return num_outputs; }
    
    /**
     * @brief Sets the name identifier for this component
     * 
     * @param name The new name for this component
     */
    void set_component_name(const std::string& name);
    
    /**
     * @brief Gets the name identifier for this component
     * 
     * @return The component's name
     */
    const std::string& get_component_name() const { return component_name; }

protected:
    /**
     * @brief Initializes input and output arrays based on num_inputs and num_outputs
     * 
     * Should be called by subclass constructors after setting num_inputs and num_outputs
     */
    void initialize_IO_arrays();
    
    /**
     * @brief Allocates input and output arrays without initializing values
     * 
     * Used by composite components that will route outputs to internal components
     */
    void allocate_IO_arrays();

    /**
     * @brief Name identifier for this component
     */
    std::string component_name = "component";
    
    /**
     * @brief Number of inputs for this component
     */
    uint16_t num_inputs = 0;
    
    /**
     * @brief Number of outputs for this component
     */
    uint16_t num_outputs = 0;
    
    /**
     * @brief Array of pointers to input boolean signals from upstream components
     * Subclasses allocate based on num_inputs static constant
     */
    bool** inputs = nullptr;
    
    /**
     * @brief Array of output boolean signals produced by this component
     * Subclasses allocate based on num_outputs static constant
     */
    bool* outputs = nullptr;
    
    /**
     * @brief Pointers to downstream components that depend on this component's outputs
     */
    std::vector<Component*> downstream_components;
};

