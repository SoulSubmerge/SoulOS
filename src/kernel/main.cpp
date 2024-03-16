#include <kernel/kernel.h>

const unsigned int magic = SOUL_MAGIC;
char message[] = "Hello SoulOS C++...\n\r";
char buf[1024];



class AAA
{
public:
    AAA(){};
    void www()
    {
        char *video = (char*)0xb8000;
        for(int i = 0; i < sizeof(message); i++)
        {
            video[i * 2] = message[i];
        }
    }
};


extern "C" void KernelInit()
{
    bool a = true;
    AAA aaa;
    aaa.www();
}