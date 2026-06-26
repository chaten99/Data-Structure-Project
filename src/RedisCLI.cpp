#include "RedisCLI.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cctype>

void RedisCLI::run() {
    std::string line;
    std::cout << "Redis Lite\n";
    std::cout << "Type 'help' for commands, or 'exit' to quit.\n";

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
        else if (cmd == "HELP") {
            std::cout << "\n";
            std::cout << "  +-----------------------------------------------------------+\n";
            std::cout << "  |                     REDIS LITE HELP                       |\n";
            std::cout << "  +-----------------------------------------------------------+\n";
            std::cout << "  | SET key value  - Set the string value of a key            |\n";
            std::cout << "  | GET key        - Get the value of a key                   |\n";
            std::cout << "  | DEL key        - Delete a key                             |\n";
            std::cout << "  | EXISTS key     - Determine if a key exists                |\n";
            std::cout << "  | DBSIZE         - Return the number of keys in the db      |\n";
            std::cout << "  | FLUSHDB        - Remove all keys from the db              |\n";
            std::cout << "  | HELP           - Show this help menu                      |\n";
            std::cout << "  | EXIT / QUIT    - Exit the CLI                             |\n";
            std::cout << "  +-----------------------------------------------------------+\n\n";
        }
        else if (cmd == "DBSIZE") {
            std::cout << "(integer) " << redisData.size() << "\n";
        }
        else if (cmd == "FLUSHDB") {
            redisData.clear();
            std::cout << "OK\n";
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
        else if (cmd == "DEL") {
            ss >> key;

            if (key.empty()) {
                std::cout << "(error) ERR wrong number of arguments for 'del' command\n";
            } else {
                try {
                    redisData.remove(key);
                    std::cout << "(integer) 1\n";
                } catch (const std::out_of_range&) {
                    std::cout << "(integer) 0\n";
                }
            }
        }
        else if (cmd == "EXISTS") {
            ss >> key;

            if (key.empty()) {
                std::cout << "(error) ERR wrong number of arguments for 'exists' command\n";
            } else {
                if (redisData.contains(key)) {
                    std::cout << "(integer) 1\n";
                } else {
                    std::cout << "(integer) 0\n";
                }
            }
        }
        else {
            std::cout << "(error) ERR unknown command '" << cmd << "'\n";
        }
    }
}