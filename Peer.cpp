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
    // Inicializa os vizinhos na lista do servidor UDP
    udp_server.setUDPNeighbors(neighbors);

    // Carrega os chunks locais do peer
    file_manager.loadLocalChunks();

    // Inicia o servidor TCP em uma thread separada (descomentado para funcionalidade futura)
    std::thread tcp_thread(&TCPServer::run, &tcp_server);

    // Inicia o servidor UDP em uma thread separada
    std::thread udp_thread(&UDPServer::run, &udp_server);

    // Espera a thread do servidor UDP (join), comentado para o servidor TCP
    tcp_thread.join();
    udp_thread.join();
}

/**
 * @brief Inicia a busca por um arquivo na rede.
 */
void Peer::searchFile(const std::string& metadata_file) {
    auto [file_name, total_chunks, initial_ttl] = file_manager.loadMetadata(metadata_file);

    // Verifica se a leitura foi bem-sucedida
    if (total_chunks != -1 && initial_ttl != -1) {
       // nicializa a estrutura responsável por armazenar as informações de número total de chunks para um arquivo
        file_manager.initializeFileChunks(file_name, total_chunks);

        // Inicializa a estrutura responsável por armazenar informações de localização dos chunks
        file_manager.initializeChunkLocationInfo(file_name);

        // Começa a descoberta dos chunks
        discoverAndRequestChunks(file_name, total_chunks, initial_ttl);
    }
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
