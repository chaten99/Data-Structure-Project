#include "RedisCLI.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cctype>

void RedisCLI::run() {
    std::string line;
    std::cout << "Redis Lite\n";
    std::cout << "Type 'exit' to quit.\n";

    while (true) {
        std::cout << "redis> ";
        if (!std::getline(std::cin, line)) break;

        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cmd, key, value;
        ss >> cmd;

        for (char &c : cmd) {
            c = std::toupper(c);
        }

        if (cmd == "EXIT" || cmd == "QUIT") {
            break;
        }
        else if (cmd == "SET") {
            ss >> key;
            std::getline(ss >> std::ws, value);

            if (key.empty() || value.empty()) {
                std::cout << "(error) ERR wrong number of arguments for 'set' command\n";
            } else {
                redisData.set(key, value);
                std::cout << "OK\n";
            }
        }
        else if (cmd == "GET") {
            ss >> key;

            if (key.empty()) {
                std::cout << "(error) ERR wrong number of arguments for 'get' command\n";
            } else {
                try {
                    std::cout << "\"" << redisData.get(key) << "\"\n";
                } catch (const std::out_of_range&) {
                    std::cout << "(nil)\n";
                }
            }
        }
        else {
            std::cout << "(error) ERR unknown command '" << cmd << "'\n";
        }
    }
}