

#ifndef _BASIC_COMPONENTS_
#define _BASIC_COMPONENTS_

#include <cstdint>

/**
 * @brief Abstract class that is inherited by ALL components of any complexity
 */
class Component
{
    private:
        uint16_t num_downstream_components = 0;
        Component** downstream_components;
        
        /**
         * @brief runs internal calculations and updates outputs based on current inputs
         */
        virtual void evaluate() = 0;
        
    public:
        static const uint16_t num_inputs = 0;
        static const uint16_t num_outputs = 0;
        const bool* const * output;
        /**
         * @brief calls self.evaluate and signals all downstream components to update themselves
         */
        void update();
        
        /**
         * @brief connects output of an upstream device to the input of given index
         * @param upstream_output_p pointer to boolean to be used as input
         * @param input_index index of this device's inputs to attach
         * @return boolean determining if connection was successful
         */
        virtual bool connect_input(const bool* const upstream_output_p, uint16_t input_index) = 0;
        
        /**
         * @brief 
         * @param downstream_component_p 
         * @param this_output_p 
         * @param downstream_index 
         * @return 
         */
        virtual bool connect_output(Component* const downstream_component_p, const bool* const this_output_p, uint16_t downstream_index) = 0;
        
        // /**
        //  * @brief calls connect_input of downstream components to request their inputs be changed to current component's outputs
        //  */
        // virtual void connect_outputs() = 0;
        
        void print_outputs();
};

/**
 * @brief Basic component with one output and no inputs. Generates high or low
 */
class Signal_Generator :
public Component
{
    public:
        /**
         * @brief has no inputs and one output
         */
        static const uint16_t num_inputs = 0;
        static const uint16_t num_outputs = 1;
        
    private:
        uint16_t num_downstream_components = 0;
        bool internal_output[1] = {0};
        static const uint16_t max_output_components = 1;
        Component* downstream_components[max_output_components];
        
        /**
         * @brief this component has nothing to evaluate. Function does nothing
         */
        void evaluate() override {;}; // does nothing
        
    public:
        const bool* const output[num_outputs] = {&internal_output[0]};
        Signal_Generator();
        
        /**
         * @brief connects output of an upstream device to the input of given index
         * @param upstream_output_p pointer to boolean to be used as input
         * @param input_index index of this device's inputs to attach
         * @return boolean determining if connection was successful
         */
        bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override; 
        
        /**
         * @brief connects one given output of this class to an input of another component
         * @param downstream_component_p pointer to component which will receive this output as input
         * @param this_output_p pointer to this class' output to use as input of receiving class
         * @param downstream_index index of the input of the receiving class to connect this to
         * @return boolean determining if connection was successful 
         */
        bool connect_output(Component* const downstream_component_p, const bool* const this_output_p, uint16_t downstream_index) override;
        
        /**
         * @brief turns outptu[0] high (true)
         */
        void go_high();
        /**
         * @brief turns outptu[0] low (false)
         */
        void go_low();
};

/**
 * Buffer class can be used to connect one logical output to up to 16 inputs
 */
class Buffer :
public Component
{
    public:
        static const uint16_t num_inputs = 1;
        static const uint16_t num_outputs = 16;
        
    private:
        const bool* input[1];
        bool internal_output[num_outputs];
        uint16_t num_connected_output_devices = 0;
        Component* downstream_components[num_outputs];
        
        /**
         * @brief this component has nothing to evaluate. Function does nothing
         */
        void evaluate() override;
        
    public:
    
        const bool* const output[num_outputs] = {&internal_output[0]};
        
        /**
         * @brief class constructor
         */
        Buffer(){;};
        
        bool connect_input(bool* upstream_output_p, uint16_t input_index);
        
        /**
         * @brief connects output of an upstream device to the input of given index
         * @param upstream_output_p pointer to boolean to be used as input
         * @param input_index index of this device's inputs to attach
         * @return boolean determining if connection was successful
         */
        bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override; 
        
        /**
         * @brief connects one given output of this class to an input of another component
         * @param downstream_component_p pointer to component which will receive this output as input
         * @param this_output_p pointer to this class' output to use as input of receiving class
         * @param downstream_index index of the input of the receiving class to connect this to
         * @return boolean determining if connection was successful 
         */
        bool connect_output(Component* const downstream_component_p, const bool* const this_output_p, uint16_t downstream_index) override;
};

// class Inverter :
// public Component
// {
//     private:
//         bool* input[1];
//         bool output[1];
//         const static int num_downstream_components = 1;
//         Component* downstream_components[num_downstream_components];
//         void evaluate();
        
//     public:
//         const uint16_t num_inputs = 1;
//         const uint16_t num_outputs = 1;
//         // void update();
// };



#endif