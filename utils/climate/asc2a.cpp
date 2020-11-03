
#include <iostream>
#include <fstream>
#include <sstream>
#include <climits>
#include <string>

#include "texture.hxx"

#define NCOLS 4320 
#define NROWS 2160 

enum STR2INT_ERROR { SUCCESS, OVERFLOW, UNDERFLOW, INCONVERTIBLE };

STR2INT_ERROR str2int (int &i, char const *s, int base = 0)
{
    char *end;
    long  l;
    errno = 0;
    l = strtol(s, &end, base);
    if ((errno == ERANGE && l == LONG_MAX) || l > INT_MAX) {
        std::cerr << "OVERFLOW" << std::endl;
        return OVERFLOW;
    }
    if ((errno == ERANGE && l == LONG_MIN) || l < INT_MIN) {
        std::cerr << "UNDERFLOW" << std::endl;
        return UNDERFLOW;
    }
    if (*s == '\0' || end == s) {
        std::cerr << "INCONVERTIBLE: " << s << std::endl;
        return INCONVERTIBLE;
    }
    i = l;
    return SUCCESS;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        printf("Uage: %s <infile>.asc <outfile>.rgb\n\n", argv[0]);
        exit(-1);
    }

    const char *infile = argv[1];
    const char *outfile = argv[2];

    SGTexture texture(NCOLS, NROWS);
    std::ifstream file(infile);
    std::string str; 

    texture.prepare(NCOLS, NROWS);
    texture.set_colors(1);

    GLuint row = 0;
    unsigned int line = 0;
    while (std::getline(file, str))
    {
        GLuint col = 0;
        int num = 0;

        if (++line < 7) continue;

        std::istringstream iss(str);
        std::string token;
        while (std::getline(iss, token, ' '))
        {
            if (str2int(num, token.c_str(), 10) == SUCCESS)
            {
                GLubyte val = (num == 32) ? 0 : 4*num;
                texture.set_pixel(col++, NROWS-row, &val);
            }
        }	
        row++;
    }

    texture.write_texture(outfile);
}
