#include <bits/stdc++.h>
using namespace std;

void fixBuffer(char *buff, int ID)
{
    if (ID >= 10)
    {
        int l = ID % 10;
        int h = (ID / 10) % 10;

        buff[3] = static_cast<char>(h + '0');
        buff[4] = static_cast<char>(l + '0');
    }
}

int main()
{
    string s;
    getline(cin, s);

    char buff[1000];

    memcpy(buff, s.c_str(), s.length());
    fixBuffer(buff, 12);
    cout << "DONE\n";
}