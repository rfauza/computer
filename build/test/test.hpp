class Test
{
    private:
        // private variable modifiable by this instance of this class
        int private_int;
    public:
        // public pointer to private variable, makes it visible but not modifiable by any external instance
        const int* const pulic_int_p = &private_int;;
        // initializer
        Test(void);
        // internally increments private variable. any attempt to modify the variable or pointer by any other class results in a compiler error
        void increment();
};