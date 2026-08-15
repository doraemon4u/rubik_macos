/**
 * min2phaseCXX Copyright (C) 2022 Borgo Federico
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 *
 * This file contains the main functions for the remote server.
 */

#ifndef MIN2PHASE_HTTP_H
#define MIN2PHASE_HTTP_H

#include <cstdint>
#include <string>

namespace min2phase{ namespace http{

    /**
     * Remote cube solver. It connects form a remote cube solver server and
     * ask the solution of the cube.
     *
     * @see: the local solver documentation
     *
     * @param ip        : the IP of the server.
     * @param port      : the pot server
     * @param facelet   : the order of colors like the cube above.
     * @param maxDepth  : the max of moves used to solve the cube, from 20 to 31.
     * @param probeMax  : the max of cube explored in phase 2.
     * @param probeMin  : the min of cube explored in phase 2.
     * @param verbose   : the format for the output strings.
     * @param usedMoves : the pointer to store the number of moves used.
     * @param time      : the string pointer to store the time used to solve the cube.
     * @return          : the moved necessary to solve the cube.
     */
    std::string webSolver(const std::string& ip, int32_t port,
                          const std::string &facelets, int8_t maxDepth,
                          int32_t probeMax, int32_t probeMin,
                          int8_t verbose, uint8_t* usedMoves,
                          std::string *time);


    /**
     * This is used to init and http server.
     *
     * @warning: this function will abilitate an handler for the zombie process,
     *           do not register another handler with SIGCHLD.
     *
     * @param port      : the port of the server.
     * @pram mReq       : (Optional) the max number of requests.
     * @return          : true if it was done correctly, false if not.
     */
    extern bool init(uint16_t port, uint16_t mReq);

    /**
     * This stops the solver server. It also remove the SIGCHLD signal handler.
     *
     * @return          : true if it was done correctly, false if not.
     */
    extern bool stop();

}   }

#endif //MIN2PHASE_HTTP_H
