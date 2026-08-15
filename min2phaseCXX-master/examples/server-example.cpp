/**
 * min2phaseCXX Copyright (C) 2022 Borgo Federico
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 *
 * This file contains an example of the solver server.
 */

#include <min2phase/min2phase.h>
#include <min2phase/tools.h>

#include <iostream>
#include <unistd.h>

/**
 * Continue sleeping even when the sleep() is interrupted by a signal.
 *
 * @param time_s        : the time in seconds to wait.
 */
void resSleep(time_t time_s){
    time_t time_tmp = time_s;
    const time_t b_time = time(nullptr);

    do {
        time_tmp = time_tmp - (time(nullptr) - b_time);
        sleep(time_tmp);
    } while (time(nullptr)-b_time < time_s);
}

int main(int argc, char* argv[]){
    const int32_t port = 5004;
    std::string a, timeSol, sol;
    uint8_t usedMoves;

    min2phase::loadFile("coords.m2pc");
    min2phase::tools::setRandomSeed(time(nullptr));

    if(min2phase::server(port))
        std::cout << "Initialized" << std::endl;
    else{
        std::cout << "Error\n";
        return 0;
    }

    //wait before request the solver(it's not necessary)
    resSleep(2);

    //solve the cube
    sol = min2phase::webSearch("127.0.0.1", port, min2phase::tools::randomCube(), 21, 1000000, 0, min2phase::USE_SEPARATOR, &usedMoves, &timeSol);
    std::cout << "Web Solver: " << sol << "  " << (int32_t)usedMoves << " moves  " << timeSol << std::endl;

    //wait to stop the server
    std::cin.clear();
    std::cin >> a;

    min2phase::stop();

    return 0;
}