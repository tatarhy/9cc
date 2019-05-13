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
try  21 'main(){5+20-4;}'
try   2 'main(){3-2+1;}'
try  41 'main () { 12 + 34 - 5 ; }'
try  47 'main(){5+6*7;}'
try  15 'main(){5*(9-6);}'
try   4 'main(){(3+5)/2;}'
try   3 'main(){a=3;}'
try   3 'main(){a=3;a;}'
try   1 'main(){a=b=1;b;}'
try  16 'main(){a=11;aa=a+5;}'
try   1 'main(){1==1;}'
try   1 'main(){3!=2;}'
try   0 'main(){3==2;}'
try   1 'main(){5*(9-6)==5+2*5;}'
try   3 'main(){foo(5, 2);}'
try   4 'main(){a=3;b=1;bar=foo(6,foo(a,b));bar;}'
try  42 'bar(){42;}main(){bar();}'
try  13 'bar(){5+4;}main(){bar()+4;}'
try  12 'bar(a,b){a+b;}main(){bar(5,4)+3;}'
try 108 'main(){a=13;b=57;if(a==13)b=108;b;}'
try  57 'main(){a=13;b=57;if(a==7)b=108;b;}'

echo OK
