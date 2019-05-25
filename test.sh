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

try   0 'main(){0;}'
try  42 'main(){42;}'
try  11 'main(){-5+20-4;}'
try   2 'main(){3-2+1;}'
try  41 'main () { 12 + 34 - 5 ; }'
try  47 'main(){5+6*7;}'
try  15 'main(){5*(9-6);}'
try   4 'main(){(3+5)/2;}'
try   3 'main(){int a;a=3;return a;}'
try   3 'main(){int a;a=3;a;}'
try   3 'main(){int _a;_a=3;_a;}'
try   3 'main(){int _a0;_a0=3;_a0;}'
try   3 'main(){int m_A0b;m_A0b=3;m_A0b;}'
try   1 'main(){int a;int b;a=b=1;b;}'
try  16 'main(){int a;int aa;a=11;aa=a+5;}'
try   1 'main(){1==1;}'
try   1 'main(){3!=2;}'
try   0 'main(){3==2;}'
try   0 'main(){2<2;}'
try   1 'main(){2<3;}'
try   0 'main(){2>2;}'
try   1 'main(){2>1;}'
try   0 'main(){3<=2;}'
try   1 'main(){2<=2;}'
try   0 'main(){1>=2;}'
try   1 'main(){2>=2;}'
try   4 'main(){(2<=3)+(3<=3)+(3>=3)+(3>=2);}'
try   1 'main(){5*(9-6)==5+2*5;}'
try   3 'main(){foo(5, 2);}'
try   4 'main(){int a;int b;int bar;a=3;b=1;bar=foo(6,foo(a,b));bar;}'
try  42 'bar(){42;}main(){bar();}'
try  13 'bar(){5+4;}main(){bar()+4;}'
try  12 'bar(a,b){a+b;}main(){bar(5,4)+3;}'
try  70 'main(){int a;int b;a=13;b=57;return a+b;b;}'
try  13 'main(){int returnn;returnn=13;}'
try  13 'main(){int iff;iff=13;}'
try 108 'main(){int a;int b;a=13;b=57;if(a==13)b=108;b;}'
try  57 'main(){int a;int b;a=13;b=57;if(a==7)b=108;b;}'
try 108 'main(){int a;int b;a=13;b=57;if(a==13)if(b==57)b=108;b;}'
try  70 'main(){int a;int b;a=13;b=57;if(a==12){a=54;b=108;}return a+b;}'
try 162 'main(){int a;int b;a=13;b=57;if(a==13){a=54;b=108;}return a+b;}'
try  55 'main(){int sum;int i;sum=0;i=1;while(i<=10){sum=sum+i;i=i+1;}return sum;}'

echo OK
