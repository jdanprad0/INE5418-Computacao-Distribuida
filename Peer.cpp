#include "Peer.h"
#include "Utils.h"
#include <thread>
#include <iostream>
#include <fstream>

/**
 * @brief Construtor da classe Peer.
 */
Peer::Peer(int id, const std::string& ip, int udp_port, int tcp_port, int transfer_speed, std::vector<std::tuple<std::string, int>> neighbors)
    : id(id), ip(ip), udp_port(udp_port), tcp_port(tcp_port), transfer_speed(transfer_speed), neighbors(neighbors),
      file_manager(std::to_string(id)), udp_server(ip, udp_port, id, transfer_speed, file_manager),
      tcp_server(ip, tcp_port, transfer_speed, file_manager) {
    // Carrega os chunks locais na inicialização
    file_manager.loadLocalChunks();
    udp_server.setUDPNeighbors(neighbors);
}

/**
 * @brief Inicia os servidores UDP e TCP.
 */
void Peer::start() {
    logMessage(LogType::INFO, "Peer " + std::to_string(id) + " inicializado.");
 
    // Inicia o servidor UDP em uma thread separada
    std::thread udp_thread(&UDPServer::run, &udp_server);

    // Inicia o servidor TCP em uma thread separada
    //std::thread tcp_thread(&TCPServer::run, &tcpServer);

    // Espera as threads
    udp_thread.join();
    //tcp_thread.join();
}

/**
 * @brief Inicia a busca por um arquivo na rede.
 */
void Peer::searchFile(const std::string& metadata_file) {
    // Lê o arquivo de metadados
    std::string file_path = "./src/" + metadata_file;
    std::ifstream meta_file(file_path);
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

    file_manager.initializeChunkResponses(file_name, total_chunks);

    // Monta um Peer Info
    PeerInfo original_sender_info(std::string(ip), udp_port);

    // Inicia o processo de descoberta do arquivo
    udp_server.sendDiscoveryMessage(file_name, total_chunks, initial_ttl, original_sender_info);
}
