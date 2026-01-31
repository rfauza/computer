
#include <iostream>
#include "./basic_components.hpp"

using namespace std;


int main(int argc, char** argv)
{
    Signal_Generator siggen1;
    Buffer buff;
    bool signal = false;
    const bool* const bool_ptr = &signal;
    
    buff.connect_input(siggen1.output[0], 0);
    cout << "siggen1 output before update: " << *siggen1.output[0] << endl;
    buff.update();
    siggen1.connect_input(bool_ptr, 0);
    cout << "siggen1 output: " << *siggen1.output[0] << endl;
    siggen1.print_outputs();
    cout << "buff output: " << *buff.output[0] << endl;
    buff.print_outputs();
    siggen1.go_high();
    buff.update();
    cout << "siggen1 output: " << *siggen1.output[0] << endl;
    cout << "buff output: " << *buff.output[0] << endl;
}