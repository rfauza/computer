#include <iostream>
#include "components/AND_Gate.hpp"
#include "components/OR_Gate.hpp"
#include "components/Inverter.hpp"
#include "components/Buffer.hpp"
#include "components/Signal_Generator.hpp"
#include "components/NAND_Gate.hpp"
#include "components/NOR_Gate.hpp"
#include "components/XOR_Gate.hpp"
#include "device_components/Half_Adder.hpp"
#include "device_components/Full_Adder.hpp"
#include "device_components/Full_Adder_Subtractor.hpp"
#include "device_components/Memory_Bit.hpp"
#include "device_components/Flip_Flop.hpp"
#include "devices/Bus.hpp"
#include "devices/Register.hpp"
#include "devices/Adder.hpp"
#include "devices/Adder_Subtractor.hpp"
#include "utilities/utilities.hpp"

int main()
{
    Register device = Register(4);
    
    test_component(&device, "100000");
    test_component(&device, "100010");
    test_component(&device, "010001");
    test_component(&device, "010010");
    test_component(&device, "000001");
    
    return 0;
}


