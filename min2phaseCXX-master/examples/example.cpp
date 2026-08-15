/**
 * min2phaseCXX Copyright (C) 2022 Borgo Federico
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 *
 * This file contains an example of the solver.
 */

#include <chrono>
#include <iostream>
#include <string>

#include <min2phase/min2phase.h>
#include <min2phase/tools.h>

int main(int argc, char *argv[]){
    uint8_t movesUsed;
    min2phase::tools::setRandomSeed(time(nullptr));

    auto start = std::chrono::high_resolution_clock::now();
    //min2phase::init();
    min2phase::loadFile("coords.m2pc");
    //min2phase::writeFile("coords.m2pc");
    auto end = std::chrono::high_resolution_clock::now();

    std::cout <<  std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "ms\n";

    start = std::chrono::high_resolution_clock::now();
    std::cout << min2phase::solve(min2phase::tools::randomCube(), 21, 1000000, 0, min2phase::APPEND_LENGTH | min2phase::USE_SEPARATOR | min2phase::INVERSE_SOLUTION, &movesUsed) << std::endl;
    end = std::chrono::high_resolution_clock::now();

    std::cout <<  std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "ms\n";

    return 0;
}