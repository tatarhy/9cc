#!/bin/bash

try() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    gcc -c -p foo.o foo.c
    gcc -c -o tmp.o tmp.s
    gcc -o tmp foo.o tmp.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$expected expected, but got $actual"
        exit 1
    fi
}

try   0 'int main(){return 0;}'
try  42 'int main(){return 42;}'
try  11 'int main(){return -5+20-4;}'
try   2 'int main(){return 3-2+1;}'
try  41 'int main () { return 12 + 34 - 5 ; }'
try  47 'int main(){return 5+6*7;}'
try  15 'int main(){return 5*(9-6);}'
try   4 'int main(){return (3+5)/2;}'
try   3 'int main(){int a;a=3;return a;}'
try   3 'int main(){int a;a=3;return a;}'
try   3 'int main(){int _a;_a=3;return _a;}'
try   3 'int main(){int _a0;_a0=3;return _a0;}'
try   3 'int main(){int m_A0b;m_A0b=3;return m_A0b;}'
try   1 'int main(){int a;int b;a=b=1;return b;}'
try  16 'int main(){int a;int aa;a=11;aa=a+5;return aa;}'
try   1 'int main(){return 1==1;}'
try   1 'int main(){return 3!=2;}'
try   0 'int main(){return 3==2;}'
try   0 'int main(){return 2<2;}'
try   1 'int main(){return 2<3;}'
try   0 'int main(){return 2>2;}'
try   1 'int main(){return 2>1;}'
try   0 'int main(){return 3<=2;}'
try   1 'int main(){return 2<=2;}'
try   0 'int main(){return 1>=2;}'
try   1 'int main(){return 2>=2;}'
try   4 'int main(){return (2<=3)+(3<=3)+(3>=3)+(3>=2);}'
try   1 'int main(){return 5*(9-6)==5+2*5;}'
try   3 'int main(){return foo(5, 2);}'
try   4 'int main(){int a;int b;int bar;a=3;b=1;bar=foo(6,foo(a,b));return bar;}'
try  42 'int bar(){42;}int main(){return bar();}'
try  13 'int bar(){5+4;}int main(){return bar()+4;}'
try  12 'int bar(int a,int b){a+b;}int main(){return bar(5,4)+3;}'
try  70 'int main(){int a;int b;a=13;b=57;return a+b;b;}'
try  13 'int main(){int returnn;returnn=13;return returnn;}'
try  13 'int main(){int iff;iff=13;}'
try 108 'int main(){int a;int b;a=13;b=57;if(a==13)b=108;return b;}'
try  57 'int main(){int a;int b;a=13;b=57;if(a==7)b=108;return b;}'
try 112 'int main(){int a;int b;a=13;b=57;if(a==7)b=108;else b=112;return b;}'
try 108 'int main(){int a;int b;a=13;b=57;if(a==13)if(b==57)b=108;return b;}'
try  70 'int main(){int a;int b;a=13;b=57;if(a==12){a=54;b=108;}return a+b;}'
try 162 'int main(){int a;int b;a=13;b=57;if(a==13){a=54;b=108;}return a+b;}'
try  55 'int main(){int sum;int i;sum=0;i=1;while(i<=10){sum=sum+i;i=i+1;}return sum;}'
try   3 'int main(){int x;x=3;int *y;y=&x;return *y;}'

echo OK
