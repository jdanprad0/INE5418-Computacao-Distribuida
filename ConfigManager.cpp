#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

/**
 * @brief Carrega a topologia da rede a partir do arquivo.
 */
std::map<int, std::vector<int>> ConfigManager::loadTopology() {
    std::string file_path = Constants::TOPOLOGY_PATH;
    std::ifstream file(file_path); // Abre o arquivo para leitura
    std::map<int, std::vector<int>> topology; // Mapa para armazenar a topologia

    // Verifica se o arquivo foi aberto corretamente
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de topologia." << std::endl;
        return topology; // Retorna um mapa vazio em caso de erro
    }

    std::string line;
    // Lê cada linha do arquivo
    while (std::getline(file, line)) {
        std::stringstream ss(line); // Cria um fluxo de string para processamento
        int node_id;
        char colon; // Variável para capturar o caractere ':' no formato

        // Lê o ID do nodo e o caractere ':'
        ss >> node_id >> colon;

        std::vector<int> neighbors; // Vetor para armazenar os vizinhos
        std::string neighbor_list;
        
        // Lê a lista de vizinhos após o caractere ':'
        std::getline(ss, neighbor_list);
        std::stringstream ss_neighbors(neighbor_list); // Fluxo para processar os vizinhos

        std::string neighbor;
        // Lê cada vizinho separado por vírgula
        while (std::getline(ss_neighbors, neighbor, ',')) {
            int neighbor_id = std::stoi(neighbor); // Converte o vizinho para inteiro
            neighbors.push_back(neighbor_id); // Adiciona ao vetor de vizinhos
        }

        topology[node_id] = neighbors; // Mapeia o nodo aos seus vizinhos
    }

    file.close(); // Fecha o arquivo após a leitura
    return topology; // Retorna o mapa de topologia
}

/**
 * @brief Carrega as configurações dos peers a partir do arquivo.
 */
std::map<int, std::tuple<std::string, int, int>> ConfigManager::loadConfig() {
    std::string file_path = Constants::CONFIG_PATH;
    std::ifstream file(file_path); // Abre o arquivo para leitura
    std::map<int, std::tuple<std::string, int, int>> config; // Mapa para armazenar as configurações

    // Verifica se o arquivo foi aberto corretamente
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de configuração." << std::endl;
        return config; // Retorna um mapa vazio em caso de erro
    }

    std::string line;
    // Lê cada linha do arquivo
    while (std::getline(file, line)) {
        std::stringstream ss(line); // Cria um fluxo de string para processamento
        int node_id;
        char colon, comma2; // Variáveis para capturar ':' e ','
        std::string ip;
        int udp_port;
        int speed;

        // Lê o ID do nodo e o caractere ':'
        ss >> node_id >> colon;

        // Captura o IP e ignora a vírgula
        std::getline(ss, ip, ','); 
        ip = trim(ip); // Remove espaços em branco do IP

        // Lê os campos restantes (udp_port e speed)
        ss >> udp_port >> comma2 >> speed;

        // Armazena os dados no mapa
        config[node_id] = std::make_tuple(ip, udp_port, speed);
    }

    file.close(); // Fecha o arquivo após a leitura
    return config; // Retorna o mapa de configurações
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
