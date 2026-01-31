
#include <iostream>
#include "./test.hpp"

using namespace std;


int main(int argc, char** argv)
{
    Test test;
    cout << *test.pulic_int_p << endl;
    test.increment();
    cout << *test.pulic_int_p << endl;
}