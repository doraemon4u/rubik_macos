/**
 * min2phaseCXX Copyright (C) 2022 Borgo Federico
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 *
 * This file contains the implementation of all the function
 * used for the remote server solver.
 */

#include <min2phase/min2phase.h>
#include <min2phase/tools.h>

#include "http.h"

#include<cstring>
#include <string>
#include <iostream>
#include <thread>
#include <sstream>

#ifdef linux

#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include <wait.h>

namespace min2phase{ namespace http {
        const uint8_t MSG_SIZE = 255;
        std::thread* server = nullptr;
        int32_t socketfd;
        bool isRunning = false;

        //SIGCHLD handler and cleaner
        void childCleaner(int32_t s){
            int32_t status;
            wait(&status);

            MIN2PHASE_OUTPUT(std::string("Child output: ") + std::to_string(status));

            if(!isRunning){
                signal(SIGCHLD, SIG_DFL);
                MIN2PHASE_OUTPUT("Signal event removed.")
            }
        }

        //convert string and compute the solution
        std::string solveCube(const std::string& args){
            int32_t pos, probeMax, probeMin;
            int8_t moves, verbose;
            uint8_t movesUsed = 0;
            std::string facelets;
            std::stringstream output;

            try{
                //c=YYWOYYGBOGGOOBRWBGRRYOOWWRROWBOWWBYRRGYWGYYBBGRBGRGWBOm=21p=10000P=0v=01
                pos = args.find('m')+2;
                facelets = args.substr(3, args.find('m') -4);
                moves = std::stoi(args.substr(pos, args.find('p')-pos-1));
                pos = args.find('p')+2;
                probeMax = std::stoi(args.substr(pos, args.find('P')-pos-1));
                pos = args.find('P')+2;
                probeMin = std::stoi(args.substr(pos, args.find('v')-pos-1));
                pos = args.find('v')+2;
                verbose = std::stoi(args.substr(pos, args.length()-pos));

                auto start = std::chrono::high_resolution_clock::now();
                output << min2phase::solve(facelets, moves, probeMax, probeMin, verbose, &movesUsed);
                auto end = std::chrono::high_resolution_clock::now();

                output << "\n" << (int32_t) movesUsed << "\n" << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "ms\n";

            }
            catch (std::invalid_argument &e) {
                output << "malformed input\n0\n0ms\n";
            }

            return output.str();
        }

        //start server
        bool startServer(const std::string& p, uint16_t mReq){
            struct addrinfo serverAdr{}, *res;

            memset(&serverAdr, 0, sizeof(serverAdr));

            serverAdr.ai_family = AF_INET;
            serverAdr.ai_socktype = SOCK_STREAM;
            serverAdr.ai_flags = AI_PASSIVE;

            if(getaddrinfo(nullptr, p.c_str(), &serverAdr, &res) != 0)
                return false;

            socketfd = socket(AF_INET, SOCK_STREAM, 0);

            if (socketfd == -1)
                return false;

            if (bind(socketfd, res->ai_addr, res->ai_addrlen) != 0)
                return false;

            freeaddrinfo(res);

            if(listen(socketfd, mReq) != 0 )
                return false;

            MIN2PHASE_OUTPUT("HTTP Server started.")

            return true;
        }

        //respond handler
        void respond(int32_t client){
            char msg[MSG_SIZE], *lines[3];
            std::string output;

            memset((void*)msg, (int32_t)'\0', MSG_SIZE );

            if (recv(client, msg, MSG_SIZE, 0) > 0){
                lines[0] = strtok (msg, " ");
                lines[1] = strtok (nullptr, " ");
                lines[2] = strtok (nullptr, "\n");

                MIN2PHASE_OUTPUT(lines[1])

                if (strncmp(lines[0], "GET\0", 4) == 0) {

                    if (strncmp(lines[2], "HTTP/1.0", 8) != 0 && strncmp(lines[2], "HTTP/1.1", 8) != 0)
                        send(client, "HTTP/1.0 400 Bad Request\n", 25, MSG_NOSIGNAL);
                    else{
                        output = "HTTP/1.1 200 OK\n\n";

                        if(strlen(lines[1]) == 1)
                            output += "k\n";
                        else if(strcmp(lines[1], "/r") == 0)
                            output += tools::randomCube();
                        else
                            output += solveCube(lines[1]);

                        send(client, output.c_str(), output.length(), MSG_NOSIGNAL);
                        MIN2PHASE_OUTPUT("Message sent.")
                    }
                }
            }
        }

        //handle request
        void requestHandler(){
            int32_t client, e;

            while (isRunning){
                client = accept(socketfd, nullptr, nullptr);

                if (client >= 0) {

                    MIN2PHASE_OUTPUT("Request received.")

                    if(fork() == 0) {
                        respond(client);
                        e = shutdown(client, SHUT_RDWR);
                        e += close(client);
                        exit(e);
                    }
                }
            }
        }

        //remote solver
        std::string webSolver(const std::string& ip, int32_t port,
                              const std::string &facelets, int8_t maxDepth,
                              int32_t probeMax, int32_t probeMin,
                              int8_t verbose, uint8_t* usedMoves,
                              std::string *time){

            struct sockaddr_in sockAdd{};
            struct hostent* sh;
            int32_t client;
            std::stringstream ms;
            std::string sol, tmp;
            char msg[MSG_SIZE];

            if ((sh = gethostbyname(ip.c_str())) == nullptr)
                return "no host";

            memcpy(&sockAdd.sin_addr.s_addr, sh->h_addr, sh->h_length);
            sockAdd.sin_port = htons(port);
            sockAdd.sin_family = AF_INET;

            client = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

            if(client == -1)
                return "no client";

            if(connect(client, (struct sockaddr*)&sockAdd, sizeof(sockaddr_in)) == -1)
                return "unable to connect";

            //c=YYWOYYGBOGGOOBRWBGRRYOOWWRROWBOWWBYRRGYWGYYBBGRBGRGWBO&m=21&p=10000&P=0&v=1
            ms << "GET /c=" << facelets << "&m=" << (int32_t)maxDepth << "&p=" << probeMax << "&P=" << probeMin << "&v=" << (int32_t)verbose << " HTTP/1.1";

            tmp = ms.str();
            write(client, tmp.c_str(), tmp.length());

            memset((void*)msg, (int32_t)'\0', MSG_SIZE );

            if (recv(client, msg, MSG_SIZE, 0) > 0){
                std::istringstream buf(msg);

                std::getline(buf, tmp);

                if(tmp == "HTTP/1.1 200 OK"){
                    std::getline(buf, tmp);
                    std::getline(buf, sol);
                    std::getline(buf, tmp);

                    if(usedMoves != nullptr){
                        try{
                            *usedMoves = std::stoi(tmp);
                        }catch(std::invalid_argument &e){
                            *usedMoves = -1;
                        }
                    }

                    if(time != nullptr)
                        std::getline(buf, *time);
                }
                else
                    sol = "error http";
            }
            else
                sol = "no response";


            shutdown(client, SHUT_RDWR);
            close(client);

            return sol;
        }

        //start server
        bool init(uint16_t port, uint16_t mReq){
            if(server != nullptr)
                return false;

            signal(SIGCHLD, childCleaner);

            if(!startServer(std::to_string(port), mReq))
                return false;

            isRunning = true;
            server = new std::thread(requestHandler);

            return true;
        }

        //stop server
        bool stop(){
            if(!isRunning)
                return false;

            isRunning = false;

            if(shutdown(socketfd, SHUT_RDWR) != 0)
                return false;

            server->join();
            delete server;

            if(close(socketfd) != 0)
                return false;

            server = nullptr;

            return true;
        }
}   }

#else

namespace min2phase{ namespace http{

    std::string webSolver(const std::string& ip, int32_t port,
                          const std::string &facelets, int8_t maxDepth,
                          int32_t probeMax, int32_t probeMin,
                          int8_t verbose, uint8_t* usedMoves,
                          std::string *time){
        return "no supported device";
    }


    bool init(uint16_t port, uint16_t mReq){
        return false;
    }

    bool stop() {
        return false;
    }

}   }

#endif