#pragma once
#include "Component.hpp"

class Signal_Generator : public Component
{
public:
    Signal_Generator();
    ~Signal_Generator() override;
    void evaluate() override;
    
    /**
     * @brief Sets all outputs to high (true)
     */
    void go_high();
    
    /**
     * @brief Sets all outputs to low (false)
     */
    void go_low();
    
    /**
     * @brief Signal generator doesn't accept inputs - overrides to reject connections
     */
    bool connect_input(const bool* const upstream_output_p, uint16_t input_index) override;
};
