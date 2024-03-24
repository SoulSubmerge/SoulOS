#include <iostream>


int main()
{
    void *ptr = (void*)0x1000;
    unsigned int a = 0x100;
    std::cout << ptr + a << std::endl;
    return 0;
}