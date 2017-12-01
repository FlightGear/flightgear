#include "yasim-common.hpp"

namespace yasim {
    /// duplicate null-terminated string
    char* dup(const char* s)
    {
        int len=0;
        while(s[len++]);
        char* s2 = new char[len+1];
        char* p = s2;
        while((*p++ = *s++));
        s2[len] = 0;
        return s2;
    }

    /// compare null-terminated strings
    bool eq(const char* a, const char* b)
    {
        while(*a && *b && *a == *b) { a++; b++; }
        // equal if both a and b points to null chars
        return !(*a || *b);
    }
}; //namespace yasim
