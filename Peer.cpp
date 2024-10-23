#include "Peer.h"
#include "Utils.h"
#include <thread>
#include <iostream>
#include <fstream>

/**
 * @brief Construtor da classe Peer. Também inicializa os servidores UDP e TCP e o gerenciador de arquivos.
 */
Peer::Peer(int id, const std::string& ip, int udp_port, int tcp_port, int transfer_speed, const std::vector<std::tuple<std::string, int>> neighbors)
    : id(id), ip(ip), udp_port(udp_port), tcp_port(tcp_port), transfer_speed(transfer_speed), neighbors(neighbors),
      file_manager(std::to_string(id)),
      tcp_server(ip, tcp_port, id, transfer_speed, file_manager),
      udp_server(ip, udp_port, id, transfer_speed, file_manager, tcp_server) {}

/**
 * @brief Inicia os servidores UDP e TCP.
 */
void Peer::start() {
    // Carrega os chunks locais do peer
    file_manager.loadLocalChunks();

    // Inicializa os vizinhos na lista do servidor UDP
    udp_server.setUDPNeighbors(neighbors);
    
    // Inicia o servidor UDP em uma thread separada
    std::thread udp_thread(&UDPServer::run, &udp_server);

    // Inicia o servidor TCP em uma thread separada (descomentado para funcionalidade futura)
    //std::thread tcp_thread(&TCPServer::run, &tcp_server);

    // Espera a thread do servidor UDP (join), comentado para o servidor TCP
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

    // Lê os dados do arquivo de metadados
    std::getline(meta_file, file_name);
    meta_file >> total_chunks;
    meta_file >> initial_ttl;
    meta_file.close();

    // Inicializa a estrutura responsável por armazenar informações de localização dos chunks
    file_manager.initializeChunkLocationInfo(file_name, total_chunks);

    // Começa a descoberta dos chunks
    discoverAndRequestChunks(file_name, total_chunks, initial_ttl);
}

/**
 * @brief Inicia o processo de descoberta e solicitação de chunks.
 */
void Peer::discoverAndRequestChunks(const std::string& file_name, int total_chunks, int initial_ttl) {
    // Monta um PeerInfo para o peer original que está enviando a solicitação
    PeerInfo original_sender_info(ip, udp_port);

    // Envia a mensagem de descoberta de chunks via UDP
    udp_server.sendChunkDiscoveryMessage(file_name, total_chunks, initial_ttl, original_sender_info);
    
    // Espera por respostas
    udp_server.waitForResponses(file_name);
    
    // Envia solicitações de chunks aos peers que possuem partes do arquivo
    udp_server.sendChunkRequestMessage(file_name);
}
