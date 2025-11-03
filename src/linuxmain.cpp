#include <iostream>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <shared_mutex>
#include <chrono>
#include <ctime>

// Linux-specific headers
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

std::shared_mutex db_mutex;

class SentinelDB {
    std::unordered_map<std::string, std::string> mpp;
    std::ofstream logfile;

public:
    SentinelDB(const std::string &filename = "data.log") {
        logfile.open(filename, std::ios::app);
        load(filename);
    }

    void compact() {
        std::unique_lock<std::shared_mutex> lock(db_mutex);
        std::string tempFile = "data.log.tmp";
        std::ofstream temp(tempFile, std::ios::trunc);
        for (auto &it : mpp) {
            temp << "SET " << it.first << " " << it.second << "\n";
        }
        temp.close();
        logfile.close();
        std::filesystem::remove("data.log");
        std::filesystem::rename(tempFile, "data.log");
        logfile.open("data.log", std::ios::app);
        std::cout << "🧹 WAL Compaction completed successfully.\n";
    }

    void load(const std::string &filename) {
        std::ifstream in(filename);
        std::string cmd, key, value;
        while (in >> cmd) {
            if (cmd == "SET") {
                in >> key >> value;
                mpp[key] = value;
            } else if (cmd == "DEL") {
                in >> key;
                mpp.erase(key);
            }
        }
    }

    void set(const std::string &key, const std::string &value) {
        mpp[key] = value;
        logfile << "SET " << key << " " << value << "\n";
        logfile.flush();
    }

    void del(const std::string &key) {
        mpp.erase(key);
        logfile << "DEL " << key << "\n";
        logfile.flush();
    }

    std::string get(const std::string &key) {
        if (mpp.find(key) != mpp.end()) return mpp[key];
        return "(nil)";
    }

    void save() {
        std::ofstream snapshot("snapshot.db");
        for (auto &it : mpp) {
            snapshot << it.first << " " << it.second << "\n";
        }
        snapshot.close();
        std::cout << "Snapshot saved.\n";
    }
};

void handleClient(int clientSocket, SentinelDB &db) {
    std::cout << "✅ Client connected!" << std::endl;
    char buffer[1024];
    std::string commandBuffer;

    while (true) {
        std::cout << "📡 Waiting for client input..." << std::endl;
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cout << "Client disconnected.\n";
            break;
        }

        buffer[bytesReceived] = '\0';
        commandBuffer += buffer;

        // remove invalid chars
        commandBuffer.erase(std::remove_if(commandBuffer.begin(), commandBuffer.end(),
                                           [](unsigned char c) { return c == 255; }),
                            commandBuffer.end());

        size_t pos;
        while ((pos = commandBuffer.find_first_of("\r\n")) != std::string::npos) {
            std::string command = commandBuffer.substr(0, pos);
            commandBuffer.erase(0, pos + 1);
            while (!commandBuffer.empty() && (commandBuffer[0] == '\r' || commandBuffer[0] == '\n'))
                commandBuffer.erase(0, 1);

            if (command.empty()) continue;
            std::cout << "Client Says: " << command << std::endl;

            std::istringstream iss(command);
            std::string cmd, key, value;
            iss >> cmd;
            std::string reply;

            if (cmd == "SET") {
                iss >> key >> value;
                {
                    std::unique_lock<std::shared_mutex> lock(db_mutex);
                    db.set(key, value);
                }
                reply = "+OK\r\n";
            } else if (cmd == "GET") {
                iss >> key;
                {
                    std::shared_lock<std::shared_mutex> lock(db_mutex);
                    reply = db.get(key) + "\r\n";
                }
            } else if (cmd == "DEL") {
                iss >> key;
                {
                    std::unique_lock<std::shared_mutex> lock(db_mutex);
                    db.del(key);
                }
                reply = "+OK\r\n";
            } else if (cmd == "SAVE") {
                {
                    std::unique_lock<std::shared_mutex> lock(db_mutex);
                    db.save();
                }
                reply = "+Snapshot saved\r\n";
            } else if (cmd == "EXIT") {
                reply = "BYE\r\n";
                send(clientSocket, reply.c_str(), reply.size(), 0);
                std::cout << "Client requested exit.\n";
                break;
            } else {
                reply = "-ERR unknown command\r\n";
            }

            send(clientSocket, reply.c_str(), reply.size(), 0);
        }
    }

    close(clientSocket);
    std::cout << "🧵 Client thread ending.\n";
}

void autosave(SentinelDB &db, int sleeptime) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(sleeptime));
        {
            std::unique_lock<std::shared_mutex> lock(db_mutex);
            db.save();
        }
        std::time_t now = std::time(nullptr);
        std::cout << "[AUTO] Snapshot saved in background.\n" << std::ctime(&now);
    }
}

int main() {
    SentinelDB db;
    cout<<"Welcome to the server";
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed!\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed!\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        std::cerr << "Listen failed!\n";
        close(server_fd);
        return 1;
    }

    std::cout << "🚀 SentinelDB Server running on port 8080...\n";
    // std::thread(autosave, std::ref(db), 10).detach();

    sockaddr_in clientAddr{};
    socklen_t clientSize = sizeof(clientAddr);

    while (true) {
        int clientSocket = accept(server_fd, (struct sockaddr *)&clientAddr, &clientSize);
        if (clientSocket < 0) {
            std::cerr << "Accept failed!\n";
            continue;
        }

        std::thread(handleClient, clientSocket, std::ref(db)).detach();
    }

    close(server_fd);
    return 0;
}
