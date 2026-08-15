/**
 * min2phaseCXX Copyright (C) 2022 Borgo Federico
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#include <fstream>
#include <min2phase/min2phase.h>
#include "Search.h"
#include "http.h"

namespace min2phase {

    //initialization
    void init() {
        info::init();
        coords::init();
    }

    //write coords
    bool writeFile(const std::string& name){
        if(!coords::isInit())
            init();

        std::ofstream out(name);

        MIN2PHASE_OUTPUT("Writing file.")

        if(out) {
            out.write(reinterpret_cast<char *>(&coords::coords), sizeof(coords::coords_t));
            out.close();
            return true;
        }

        return false;
    }

    //load coords
    bool loadFile(const std::string& name){
        std::ifstream in(name);

        MIN2PHASE_OUTPUT("Loading file.")

        if(in){
            in.read(reinterpret_cast<char*>(&coords::coords), sizeof(coords::coords_t));
            in.close();
            info::init();
            return true;
        }
        else
            init();

        return false;
    }

    //solve the cube
    std::string solve(const std::string &facelets, int8_t maxDepth, int32_t probeMax, int32_t probeMin,
                                     int8_t verbose, uint8_t* usedMoves) {
        MIN2PHASE_OUTPUT("Init solving.")
        return Search().solve(facelets, maxDepth, probeMax, probeMin, verbose, usedMoves);
    }

    //init server
    bool server(uint16_t port, uint16_t mReq){
        return http::init(port, mReq);
    }

    //stop solver
    bool stop(){
        return http::stop();
    }

    //remote search
    std::string webSearch(const std::string& ip, int32_t port,
                          const std::string &facelets, int8_t maxDepth,
                          int32_t probeMax, int32_t probeMin,
                          int8_t verbose, uint8_t* usedMoves,
                          std::string *time){

        return http::webSolver(ip, port, facelets, maxDepth, probeMax, probeMin, verbose, usedMoves, time);
    }
}
