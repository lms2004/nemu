#include "stdio.h"
int main(int argc, char** argv)
{
    unsigned int ca= 1<<31;
 
    unsigned  char  ucb=128;
     
    unsigned  short   usc=0;
     
    usc=ca + ucb; 
     
        printf("%x\n",usc);
     
    usc=ca +(unsigned  short)ucb; 
        printf("%x\n",(unsigned  short)ucb);
        printf("%x\n",usc);
     
    usc=(unsigned char)ca + ucb;
     
        printf("%x\n",usc);
     
    usc=ca+(char)ucb;
 
    return 0;
}