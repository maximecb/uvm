#include <stdio.h>

class ObjA
{
    public:
    virtual int f() {return a;}
    int a = 1;
};

class ObjB : public ObjA
{
    public:
    virtual int f() {return b;}
    int b = 99;
};


int main()
{
    ObjA oa;
    oa.a = 10;
    ObjA * oap = &oa;
    printf("a = %d\n", oap->f());

    ObjB ob;
    ob.a = 10;
    oap = &ob;
    printf("a = %d\n", oap->f());

    return 0;
}