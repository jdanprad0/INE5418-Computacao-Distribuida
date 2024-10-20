#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

/**
 * @brief Carrega a topologia da rede a partir do arquivo.
 */
std::map<int, std::vector<int>> ConfigManager::loadTopology(const std::string& file_name) {
    std::string file_path = "./src/" + file_name;
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
std::map<int, std::tuple<std::string, int, int>> ConfigManager::loadConfig(const std::string& file_name) {
    std::string file_path = "./src/" + file_name;
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
        char colon, comma2;
        std::string ip;
        int udp_port;
        int speed;

        // Primeiro captura o node_id e o colon (:)
        ss >> node_id >> colon;

        // Depois captura o IP, usando std::getline para ignorar a vírgula após o IP
        std::getline(ss, ip, ',');

        // Captura o restante dos campos (udp_port e speed)
        ss >> udp_port >> comma2 >> speed;

        // Armazena os dados no mapa
        config[node_id] = std::make_tuple(ip, udp_port, speed);
    }

    file.close();
    return config;
}

/**
 * @brief Expande a topologia com as informações detalhadas da configuração dos peers.
 */
std::map<int, std::vector<std::tuple<std::string, int>>> ConfigManager::expandTopology(
    const std::map<int, std::vector<int>>& topology, 
    const std::map<int, std::tuple<std::string, int, int>>& config) 
{
    // Mapa que armazenará a topologia expandida com apenas IP e porta
    std::map<int, std::vector<std::tuple<std::string, int>>> expanded_topology;
    
    // Itera sobre cada nó e seus vizinhos na topologia
    for (const auto& [node_id, neighbors] : topology) {
        // Vetor que armazenará os detalhes dos vizinhos (IP, porta)
        std::vector<std::tuple<std::string, int>> detailed_neighbors;

        // Itera sobre os vizinhos deste nó
        for (const int& neighbor_id : neighbors) {
            // Verifica se o vizinho existe na configuração
            if (config.find(neighbor_id) != config.end()) {
                // Extrai o IP e a porta do vizinho a partir da configuração
                auto [neighbor_ip, neighbor_port, _] = config.at(neighbor_id);

                // Adiciona os detalhes do vizinho (IP, porta) ao vetor
                detailed_neighbors.emplace_back(neighbor_ip, neighbor_port);
            }
        }
        // Associa a lista de vizinhos detalhados a este nó na topologia expandida
        expanded_topology[node_id] = detailed_neighbors;
    }

    // Retorna a topologia expandida com IP e porta
    return expanded_topology;
}
