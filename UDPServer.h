#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <string>
#include <map>
#include <vector>
#include <set>
#include <tuple>
#include "FileManager.h"

/**
 * @brief Classe responsável pela comunicação UDP para descoberta de arquivos.
 */
class UDPServer {
private:
    std::string ip;                                                     ///< Endereço IP do peer.
    int port;                                                           ///< Porta UDP para descoberta.
    int peer_id;                                                        ///< ID do peer.
    int sockfd;                                                         ///< Socket UDP.
    FileManager& fileManager;                                           ///< Referência ao gerenciador de arquivos. 
    std::vector<std::tuple<std::string, int>> udpNeighbors;             ///< Informações dos vizinhos (IP e Porta).
    std::map<std::string, std::set<std::string>> received_messages;     ///< Mapeia mensagens e IPs de origem.

public:
    /**
     * @brief Construtor da classe UDPServer.
     * @param ip Endereço IP do peer.
     * @param port Porta UDP para descoberta.
     * @param peer_id ID do peer.
     * @param fileManager Referência ao gerenciador de arquivos.
     */
    UDPServer(const std::string& ip, int port, int peer_id, FileManager& fileManager);

    /**
     * @brief Define informações para as conexões a serem feitas
     * @param neighbors Vizinhos do peer.
     */
    void setUDPNeighbors(const std::vector<std::tuple<std::string, int>>& neighbors);

    /**
     * @brief Inicia o servidor UDP para receber mensagens de descoberta.
     */
    void run();

    /**
     * @brief Processa a mensagem recebida em uma nova thread.
     * @param message Mensagem recebida.
     * @param direct_sender_ip IP do peer que enviou a mensagem.
     * @param direct_sender_port Porta UDP do peer que enviou a mensagem.
     */
    void processMessage(const std::string& message, const std::string& direct_sender_ip, int direct_sender_port);

    /**
     * @brief Processa a mensagem DISCOVERY recebida.
     * @param message Stream da com os dados da mensagem DISCOVERY.
     * @param direct_sender_ip IP do peer que enviou a mensagem.
     */
    void processDiscoveryMessage(std::stringstream& message, const std::string& direct_sender_ip);

    /**
     * @brief Processa a mensagem RESPONSE recebida.
     * @param message Stream com os dados da mensagem RESPONSE.
     * @param direct_sender_ip IP do peer que enviou a mensagem.
     * @param direct_sender_port Porta UDP do peer que enviou a mensagem.
     */
    void processResponseMessage(std::stringstream& message, const std::string& direct_sender_ip, int direct_sender_port);

    /**
     * @brief Envia uma resposta para o peer solicitante com os chunks disponíveis.
     * @param file_name Nome do arquivo solicitado.
     * @param requester_ip Endereço IP do peer que solicitou a mensagem.
     * @param requester_UDP_port Porta UDP do peer solicitante.
     * @return Retorna true se possui chunks para o filename e false se não possui
     */
    bool sendChunkResponse(const std::string& file_name, const std::string& requester_ip, int requester_UDP_port);

    /**
     * @brief Monta a mensagem de descoberta de um arquivo.
     * @param file_name O nome do arquivo a ser descoberto.
     * @param total_chunks Número total de chunks do arquivo.
     * @param ttl Time-to-live para o flooding de mensagens.
     * @param original_sender_ip Ip de quem deseja receber o arquivo.
     * @param original_sender_UDP_port Porta UDP de quem deseja receber o arquivo.
     * @return A string contendo a mensagem formatada de descoberta.
     */
    std::string buildDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, std::string original_sender_ip, int original_sender_UDP_port) const;

    /**
     * @brief Constrói uma mensagem de resposta contendo os chunks disponíveis.
     * @param file_name Nome do arquivo solicitado.
     * @param chunks_available Vetor contendo os IDs dos chunks disponíveis.
     * @return A string contendo a mensagem de resposta.
     */
    std::string buildChunkResponseMessage(const std::string& file_name, const std::vector<int>& chunks_available) const;

    /**
     * @brief Inicia o processo de descoberta de um arquivo.
     * @param file_name Nome do arquivo.
     * @param total_chunks Total de chunks do arquivo.
     * @param ttl Time-to-live para o flooding.
     * @param direct_sender_ip Endereço IP do peer que enviou a mensagem.
     * @param original_sender_ip Ip de quem deseja receber o arquivo.
     * @param original_sender_UDP_port Porta UDP de quem deseja receber o arquivo.
     */
    void initiateDiscovery(const std::string& file_name, int total_chunks, int ttl, std::string direct_sender_ip, std::string original_sender_ip, int original_sender_UDP_port);

    /**
     * @brief Envia uma mensagem de descoberta para os vizinhos.
     * @param message Mensagem a ser enviada.
     * @param direct_sender_ip Endereço IP do peer que enviou a mensagem.
     * @param original_sender_ip Ip de quem deseja receber o arquivo.
     */
    void sendDiscoveryMessage(const std::string& message, std::string direct_sender_ip, std::string original_sender_ip);
};

#endif // UDPSERVER_H
