#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

/**
 * @brief Carrega a topologia da rede a partir do arquivo.
 */
std::map<int, std::vector<int>> ConfigManager::loadTopology(const std::string& file_path) {
    std::ifstream file(file_path);
    std::map<int, std::vector<int>> topology;

    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de topologia." << std::endl;
        return topology;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        int node_id;
        char colon;
        ss >> node_id >> colon;

        std::vector<int> neighbors;
        std::string neighbor_list;
        std::getline(ss, neighbor_list);
        std::stringstream ss_neighbors(neighbor_list);

        std::string neighbor;
        while (std::getline(ss_neighbors, neighbor, ',')) {
            int neighbor_id = std::stoi(neighbor);
            neighbors.push_back(neighbor_id);
        }

        topology[node_id] = neighbors;
    }

    file.close();
    return topology;
}

/**
 * @brief Carrega as configurações dos peers a partir do arquivo.
 */
std::map<int, std::tuple<std::string, int, int>> ConfigManager::loadConfig(const std::string& file_path) {
    std::ifstream file(file_path);
    std::map<int, std::tuple<std::string, int, int>> config;

    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de configuração." << std::endl;
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        int node_id;
        char colon, comma1, comma2;
        std::string ip;
        int udp_port;
        int speed;

        ss >> node_id >> colon >> ip >> comma1 >> udp_port >> comma2 >> speed;

        config[node_id] = std::make_tuple(ip, udp_port, speed);
    }

    file.close();
    return config;
}

/**
 * @brief Expande a topologia com as informações detalhadas da configuração dos peers.
 */
std::map<int, std::vector<std::tuple<int, std::string, int, int>>> ConfigManager::expandTopology(
    const std::map<int, std::vector<int>>& topology, 
    const std::map<int, std::tuple<std::string, int, int>>& config) 
{
    std::map<int, std::vector<std::tuple<int, std::string, int, int>>> expanded_topology;

    for (const auto& [node_id, neighbors] : topology) {
        std::vector<std::tuple<int, std::string, int, int>> detailed_neighbors;

        for (const int& neighbor_id : neighbors) {
            if (config.find(neighbor_id) != config.end()) {
                auto [neighbor_ip, neighbor_port, neighbor_speed] = config.at(neighbor_id);
                detailed_neighbors.emplace_back(neighbor_id, neighbor_ip, neighbor_port, neighbor_speed);
            }
        }

        expanded_topology[node_id] = detailed_neighbors;
    }

    return expanded_topology;
}
