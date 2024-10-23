#ifndef PEER_H
#define PEER_H

#include <string>
#include <map>
#include <vector>
#include "FileManager.h"
#include "ConfigManager.h"
#include "UDPServer.h"
#include "TCPServer.h"

/**
 * @brief Classe que representa um peer na rede P2P.
 * 
 * Esta classe encapsula todas as funcionalidades relacionadas a um peer 
 * em uma rede peer-to-peer (P2P). Um peer pode descobrir e solicitar arquivos 
 * na rede via UDP, transferir chunks de arquivos via TCP, e gerenciar seus próprios 
 * arquivos e chunks localmente através de um `FileManager`.
 */
class Peer {
private:
    const int id;                                                       ///< Identificador único do peer (não pode ser alterado após inicialização).
    const std::string ip;                                               ///< Endereço IP atribuído ao peer (imutável após inicialização).
    const int udp_port;                                                 ///< Porta UDP usada para descoberta de arquivos.
    const int tcp_port;                                                 ///< Porta TCP usada para transferência de chunks.
    const int transfer_speed;                                           ///< Capacidade de transferência de dados do peer (em bytes/s).
    const std::vector<std::tuple<std::string, int>> neighbors;          ///< Lista de vizinhos diretos do peer, incluindo seus IPs e portas UDP.
    FileManager file_manager;                                           ///< Gerenciador responsável por lidar com os arquivos e chunks do peer.
    TCPServer tcp_server;                                               ///< Servidor TCP usado para transferir chunks de arquivos entre peers.
    UDPServer udp_server;                                               ///< Servidor UDP usado para descoberta de arquivos e peers na rede P2P.

public:
    /**
     * @brief Construtor da classe Peer.
     * 
     * Inicializa um peer na rede P2P com o ID, IP, portas UDP e TCP, velocidade de 
     * transferência e informações sobre seus vizinhos.
     * 
     * @param id Identificador do peer.
     * @param ip Endereço IP do peer.
     * @param udp_port Porta UDP para descoberta de arquivos.
     * @param tcp_port Porta TCP para transferência de chunks.
     * @param transfer_speed Capacidade de transferência em bytes/s.
     * @param neighbors Informações dos vizinhos do peer (IP, porta UDP).
     */
    Peer(int id, const std::string& ip, int udp_port, 
         int tcp_port, int transfer_speed, 
         const std::vector<std::tuple<std::string, int>> neighbors);

    /**
     * @brief Inicia os servidores UDP e TCP.
     * 
     * Ativa e inicia os servidores TCP e UDP, permitindo que o peer se comunique 
     * na rede P2P para descoberta de arquivos e transferência de chunks.
     */
    void start();

    /**
     * @brief Inicia a busca por um arquivo na rede.
     * 
     * Busca um arquivo específico na rede P2P baseado no arquivo de metadados (.p2p).
     * Utiliza o servidor UDP para descobrir peers que possuem os chunks do arquivo.
     * 
     * @param metadata_file Nome do arquivo de metadados (.p2p) usado para buscar o arquivo.
     */
    void searchFile(const std::string& metadata_file);

    /**
     * @brief Inicia o processo de descoberta e solicitação de chunks.
     * 
     * Este método envia uma mensagem de descoberta de chunks para encontrar peers 
     * que possuam partes (chunks) de um arquivo específico. Em seguida, aguarda 
     * pelas respostas e solicita os chunks disponíveis.
     * 
     * @param file_name Nome do arquivo cujos chunks estão sendo solicitados.
     * @param total_chunks Número total de chunks do arquivo.
     * @param initial_ttl Valor inicial do TTL (Time-to-Live) da mensagem de descoberta.
     */
    void discoverAndRequestChunks(const std::string& file_name, int total_chunks, int initial_ttl);
};

#endif // PEER_H
