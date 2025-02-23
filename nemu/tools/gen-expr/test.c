#include "stdio.h"
int main(int argc, char** argv)
{
    int a = 0xfffffffe; // -2
    int b = a >> 1;               //signed,   arithmetic shift right
    int c = (unsigned int)a >> 1; //unsigned, logic shift right
    printf(" b = 0x%x (%d) \n", b,b); // b = a /2 = -1
    printf(" c = 0x%x (%d) \n", c,c);
 
    unsigned int d = 0xfffffffe; // 
    unsigned int e = d >> 1;      //unsigned, logic shift right
    unsigned int f = (int)d >> 1; //signed,   arithmetic shift right
    printf(" e = 0x%x (%d) \n", e,e); 
    printf(" f = 0x%x (%d) \n", f,f);// f = d /2 = -1
 
    return 0;
}