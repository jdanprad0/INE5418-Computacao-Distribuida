#include "Peer.h"
#include <thread>
#include <iostream>
#include <fstream>

/**
 * @brief Construtor da classe Peer.
 */
Peer::Peer(int id, const std::string& ip, int udp_port, int tcp_port, int transfer_speed, std::vector<std::tuple<int, std::string, int, int>> neighbor_info)
    : id(id), ip(ip), udp_port(udp_port), tcp_port(tcp_port), transfer_speed(transfer_speed), neighbor_info(neighbor_info),
      fileManager(std::to_string(id)), udpServer(ip, udp_port, id, fileManager),
      tcpServer(ip, tcp_port, transfer_speed, fileManager) {
    // Carrega os chunks locais na inicialização
    fileManager.loadLocalChunks();
    loadUDPConnections();
}

/**
 * @brief Carrega a topologia da rede.
 */
void Peer::loadUDPConnections() {
    udpServer.setConnections(neighbor_info);
    
    // Exibe as informações dos vizinhos para depuração
    for (const auto& [id, ip, port, speed] : neighbor_info) {
        std::cout << "Vizinho ID: " << id << ", IP: " << ip << ", Porta: " << port << ", Velocidade: " << speed << std::endl;
    }
}

/**
 * @brief Inicia os servidores UDP e TCP.
 */
void Peer::start() {
    // Inicia o servidor UDP em uma thread separada
    std::thread udp_thread(&UDPServer::run, &udpServer);

    // Inicia o servidor TCP em uma thread separada
    std::thread tcp_thread(&TCPServer::run, &tcpServer);

    // Espera as threads
    udp_thread.join();
    tcp_thread.join();
}

/**
 * @brief Inicia a busca por um arquivo na rede.
 */
void Peer::searchFile(const std::string& metadata_file) {
    // Lê o arquivo de metadados
    std::ifstream meta_file(metadata_file);
    if (!meta_file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de metadados." << std::endl;
        return;
    }

    std::string file_name;
    int total_chunks;
    int initial_ttl;

    std::getline(meta_file, file_name);
    meta_file >> total_chunks;
    meta_file >> initial_ttl;
    meta_file.close();

    // Inicia o processo de descoberta do arquivo
    udpServer.initiateDiscovery(file_name, total_chunks, initial_ttl);
}
