// utilities.hpp
#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

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

int getId(char *buff){
    int ID = buff[3] - '0';
    if(buff[4] != ' '){
        ID *= 10;
        ID += (buff[4] - '0');
    }

    return ID;
}

#endif 