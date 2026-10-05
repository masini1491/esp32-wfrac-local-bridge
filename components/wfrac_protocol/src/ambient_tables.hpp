#pragma once

#include <cstdint>

// Normalized numeric data ONLY from utils.py outdoorTempList/indoorTempList,
// blues-sechseck/Mitsubishi-WF-RAC-Integration
// revision 928efd8637bcb346a4945c545ab76733eae6245c (MIT).
// Exact decimal Celsius values scaled to integer deci-Celsius; no surrounding
// upstream implementation was ported. Network OP2 lookup, NOT direct CNS.
//
// MIT License
//
// Copyright (c) 2022 jeatheak
// Copyright (c) 2026 blues-sechseck
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

namespace wfrac_protocol {
namespace {

constexpr int16_t outdoor_deci_celsius[] = {
    -500, -500, -500, -500, -500, -489, -460, -440, -420, -410, -390, -380, -370, -360, -350, -340,
    -330, -320, -310, -300, -290, -285, -280, -270, -260, -255, -250, -240, -235, -230, -225, -220,
    -215, -210, -205, -200, -195, -190, -185, -180, -175, -170, -165, -160, -155, -150, -146, -143,
    -140, -135, -130, -126, -123, -120, -115, -110, -106, -103, -100, -96, -93, -90, -86, -83,
    -80, -76, -73, -70, -66, -63, -60, -56, -53, -50, -46, -43, -40, -37, -35, -32,
    -30, -26, -23, -20, -17, -15, -12, -10, -6, -3, 0, 2, 5, 7, 10, 13,
    16, 20, 22, 25, 27, 30, 32, 35, 37, 40, 42, 45, 47, 50, 52, 55,
    57, 60, 62, 65, 67, 70, 72, 75, 77, 80, 82, 85, 87, 90, 92, 95,
    97, 100, 102, 105, 107, 110, 112, 115, 117, 120, 122, 125, 127, 130, 132, 135,
    137, 140, 142, 144, 146, 148, 150, 152, 155, 157, 160, 162, 165, 167, 170, 172,
    175, 177, 180, 182, 185, 187, 190, 192, 194, 196, 198, 200, 202, 205, 207, 210,
    212, 215, 217, 220, 222, 225, 227, 230, 232, 235, 237, 240, 242, 245, 247, 250,
    252, 255, 257, 260, 262, 265, 267, 270, 272, 275, 277, 280, 282, 285, 287, 290,
    292, 295, 297, 300, 302, 305, 307, 310, 313, 316, 320, 322, 325, 327, 330, 332,
    335, 337, 340, 343, 346, 350, 352, 355, 357, 360, 363, 366, 370, 372, 375, 377,
    380, 383, 386, 390, 393, 396, 400, 403, 406, 410, 413, 416, 420, 423, 426, 430,
};
static_assert(sizeof(outdoor_deci_celsius) / sizeof(outdoor_deci_celsius[0]) == 256U);

constexpr int16_t indoor_deci_celsius[] = {
    -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300, -300,
    -290, -280, -270, -260, -250, -240, -230, -225, -220, -210, -200, -195, -190, -180, -175, -170,
    -165, -160, -150, -145, -140, -135, -130, -125, -120, -115, -110, -105, -100, -95, -90, -86,
    -83, -80, -75, -70, -65, -60, -56, -53, -50, -45, -40, -36, -33, -30, -26, -23,
    -20, -16, -13, -10, -5, 0, 3, 6, 10, 13, 16, 20, 23, 26, 30, 32,
    35, 37, 40, 43, 46, 50, 53, 56, 60, 63, 66, 70, 72, 75, 77, 80,
    83, 86, 90, 92, 95, 97, 100, 103, 106, 110, 112, 115, 117, 120, 123, 126,
    130, 132, 135, 137, 140, 142, 145, 147, 150, 153, 156, 160, 162, 165, 167, 170,
    172, 175, 177, 180, 182, 185, 187, 190, 192, 195, 197, 200, 202, 205, 207, 210,
    212, 215, 217, 220, 222, 225, 227, 230, 232, 235, 237, 240, 242, 245, 247, 250,
    252, 255, 257, 260, 262, 265, 267, 270, 272, 275, 277, 280, 282, 285, 287, 290,
    292, 295, 297, 300, 302, 305, 307, 310, 313, 316, 320, 322, 325, 327, 330, 332,
    335, 337, 340, 342, 345, 347, 350, 353, 356, 360, 362, 365, 367, 370, 372, 375,
    377, 380, 383, 386, 390, 392, 395, 397, 400, 403, 406, 410, 412, 415, 417, 420,
    423, 426, 430, 432, 435, 437, 440, 443, 446, 450, 453, 456, 460, 462, 465, 467,
    470, 473, 476, 480, 483, 486, 490, 493, 496, 500, 503, 506, 510, 513, 516, 520,
};
static_assert(sizeof(indoor_deci_celsius) / sizeof(indoor_deci_celsius[0]) == 256U);

}  // namespace
}  // namespace wfrac_protocol
