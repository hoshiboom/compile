#include "sylib.h"
int main(){
    int a,b,c,d,e,f,g,h,i,j,k,l,m,n;
    a=1;
    b=2;
    c=3;
    d=a+b;
    e=a-b;
    f=a*b;
    g=c/a;
    d=-d;

    h=a==b;
    i=a<=b;
    j=a>b;
    
    k=a&&b;
    l=a||b;
    m=!a;
    
    n=getin();
    putin(a);

}