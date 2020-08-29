#include <stdio.h>
#include <utility>
#include <iostream>
using namespace std;

int main(){
    bitset<32> bs1("0101");
    bitset<32> bs2("0011");
    cout << "bs1=" << bs1 << endl;
    cout << "bs2=" << bs2 << endl;
    cout << "bs1&bs2=" << (bs1&bs2) << endl;
    char c = 'c';
    int i = c;
    cout << "i=" << i << endl;
    return 0;
}