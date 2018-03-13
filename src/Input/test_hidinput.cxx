// Written by James Turner, started 2017.
//
// Copyright (C) 2017  James Turner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "config.h"

#include "test_suite/helpers/globals.hxx"

#include <simgear/misc/test_macros.hxx>

#include "FGHIDEventInput.cxx"

void testValueExtract()
{
    uint8_t testDataFromSpec[4] = {0, 0xf4, 0x1 | (0x7 << 2), 0x03};
    SG_VERIFY(extractBits(testDataFromSpec, 4, 8, 10) == 500);
    SG_VERIFY(extractBits(testDataFromSpec, 4, 18, 10) == 199);

    uint8_t testData2[4] = {0x01 << 6 | 0x0f,
        0x17 | (1 << 6),
        0x3 | (0x11 << 2),
        0x3d | (1 << 6) };

    SG_VERIFY(extractBits(testData2, 4, 0, 6) == 15);
    SG_VERIFY(extractBits(testData2, 4, 6, 12) == 3421);
    SG_VERIFY(extractBits(testData2, 4, 18, 12) == 3921);
    SG_VERIFY(extractBits(testData2, 4, 30, 1) == 1);
    SG_VERIFY(extractBits(testData2, 4, 31, 1) == 0);
}

// void writeBits(uint8_t* bytes, size_t bitOffset, size_t bitSize, int value)

void testValueInsert()
{
    uint8_t buf[8];
    memset(buf, 0, 8);
    
    int a = 3421;
    int b = 3921;
    writeBits(buf, 6, 12, a);
    writeBits(buf, 18, 12, b);

    SG_VERIFY(buf[0] == 0x40);
    SG_VERIFY(buf[1] == 0x57);
    SG_VERIFY(buf[2] == (0x03 | 0x44));
    SG_VERIFY(buf[3] == 0x3d);
}

void testSignExtension()
{
    SG_VERIFY(signExtend(0x80, 8) == -128);
    SG_VERIFY(signExtend(0xff, 8) == -1);
    SG_VERIFY(signExtend(0x7f, 8) == 127);
    
    SG_VERIFY(signExtend(0x831, 12) == -1999);
    SG_VERIFY(signExtend(0x7dd, 12) == 2013);
}

int main(int argc, char* argv[])
{
    testValueExtract();
    testValueInsert();
    testSignExtension();
    
    return EXIT_SUCCESS;
}
