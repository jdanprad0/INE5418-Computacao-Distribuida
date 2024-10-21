#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <string>
#include <map>
#include <vector>
#include <set>
#include <tuple>
#include "FileManager.h"

/**
 * @brief Definição da struct PeerInfo.
 */
struct PeerInfo {
    std::string ip;  ///< Endereço IP do peer.
    int port;        ///< Porta UDP do peer.

    PeerInfo(const std::string& ip, int port) : ip(ip), port(port) {}
};

/**
 * @brief Classe responsável por gerenciar a comunicação UDP para descoberta de arquivos em uma rede P2P.
 * 
 * Esta classe implementa as funcionalidades de envio e recebimento de mensagens UDP 
 * relacionadas à descoberta de arquivos, assim como o processamento dessas mensagens. 
 */
class UDPServer {
private:
    std::string ip;                                                     ///< Endereço IP do peer atual.
    int port;                                                           ///< Porta UDP que o peer está utilizando para a comunicação.
    int peer_id;                                                        ///< Identificador único (ID) do peer.
    int sockfd;                                                         ///< Descriptor do socket UDP utilizado para a comunicação.
    FileManager& fileManager;                                           ///< Referência ao gerenciador de arquivos (armazenamento e chunks).
    std::vector<std::tuple<std::string, int>> udpNeighbors;             ///< Lista contendo os vizinhos diretos do peer (endereços IP e portas).

public:
    /**
     * @brief Construtor da classe UDPServer.
     * @param ip Endereço IP do peer.
     * @param port Porta UDP usada para a comunicação.
     * @param peer_id ID do peer na rede P2P.
     * @param file_manager Referência ao gerenciador de arquivos do peer.
     */
    UDPServer(const std::string& ip, int port, int peer_id, FileManager& file_manager);

    /**
     * @brief Define os vizinhos para o peer atual.
     * 
     * Esta função é usada para configurar os peers vizinhos com quem este peer 
     * pode se comunicar diretamente via UDP.
     * 
     * @param neighbors Vizinhos do peer (IP e Porta).
     */
    void setUDPNeighbors(const std::vector<std::tuple<std::string, int>>& neighbors);

    /**
     * @brief Inicia o servidor UDP, permitindo que o peer receba e envie mensagens.
     * 
     * Essa função ativa o loop principal para o recebimento de mensagens UDP e 
     * encaminha as mensagens recebidas para o processamento adequado.
     */
    void run();

    /**
     * @brief Processa uma mensagem recebida de outro peer.
     * 
     * A mensagem recebida será analisada e processada em uma nova thread para 
     * melhorar o desempenho e permitir a recepção simultânea de várias mensagens.
     * 
     * @param message A mensagem recebida.
     * @param direct_sender_info Informações sobre o peer que enviou diretamente a mensagem, incluindo seu endereço IP e porta UDP.
     */
    void processMessage(const std::string& message, const PeerInfo& direct_sender_info);

    /**
     * @brief Processa uma mensagem de descoberta (DISCOVERY).
     * 
     * Esta função é responsável por processar mensagens DISCOVERY, que são enviadas 
     * por peers que estão buscando um arquivo na rede.
     * 
     * @param message Stream com os dados da mensagem DISCOVERY.
     * @param direct_sender_info Informações sobre o peer que enviou diretamente a mensagem, incluindo seu endereço IP e porta UDP.
     */
    void processDiscoveryMessage(std::stringstream& message, const PeerInfo& direct_sender_info);

    /**
     * @brief Processa uma mensagem de resposta (RESPONSE).
     * 
     * Esta função é responsável por processar as respostas recebidas após um peer enviar 
     * uma solicitação de descoberta de arquivo.
     * 
     * @param message Stream com os dados da mensagem RESPONSE.
     * @param direct_sender_info Informações sobre o peer que enviou diretamente a mensagem, incluindo seu endereço IP e porta UDP.
     */
    void processResponseMessage(std::stringstream& message, const PeerInfo& direct_sender_info);

    /**
     * @brief Envia uma mensagem de descoberta (DISCOVERY) para todos os vizinhos.
     * 
     * Essa mensagem será usada para solicitar a localização de um arquivo específico na rede.
     * 
     * @param file_name Nome do arquivo que o peer deseja localizar.
     * @param total_chunks Número total de chunks que compõem o arquivo.
     * @param ttl Time-to-live para limitar o alcance do flooding.
     * @param original_sender_ip IP do peer original que solicitou a busca pelo arquivo.
     * @param original_sender_UDP_port Porta UDP do peer original que solicitou a busca.
     */
    void sendDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, std::string original_sender_ip, int original_sender_UDP_port);
    
    /**
     * @brief Envia uma resposta (RESPONSE) contendo os chunks disponíveis para um arquivo.
     * 
     * Após receber uma solicitação de descoberta, essa função envia uma resposta 
     * para o peer solicitante informando quais chunks estão disponíveis.
     * 
     * @param file_name Nome do arquivo solicitado.
     * @param requester_ip Endereço IP do peer que solicitou o arquivo.
     * @param requester_UDP_port Porta UDP do peer solicitante.
     * @return Retorna true se possui chunks disponíveis, false caso contrário.
     */
    bool sendChunkResponse(const std::string& file_name, const std::string& requester_ip, int requester_UDP_port);

    /**
     * @brief Monta a mensagem de descoberta (DISCOVERY) para envio.
     * 
     * Constrói uma string formatada contendo as informações da mensagem DISCOVERY que será enviada 
     * aos vizinhos para a busca de um arquivo.
     * 
     * @param file_name Nome do arquivo a ser descoberto.
     * @param total_chunks Número total de chunks do arquivo.
     * @param ttl Time-to-live da mensagem DISCOVERY.
     * @param original_sender_ip IP do peer original que solicitou a busca pelo arquivo.
     * @param original_sender_UDP_port Porta UDP do peer original que solicitou a busca.
     * @return String contendo a mensagem DISCOVERY formatada.
     */
    std::string buildDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, std::string original_sender_ip, int original_sender_UDP_port) const;

    /**
     * @brief Constrói uma mensagem de resposta (RESPONSE) contendo os chunks disponíveis.
     * 
     * Cria uma string formatada com as informações de quais chunks estão disponíveis 
     * para o arquivo solicitado pelo peer.
     * 
     * @param file_name Nome do arquivo solicitado.
     * @param chunks_available Vetor com os IDs dos chunks disponíveis.
     * @return String contendo a mensagem RESPONSE formatada.
     */
    std::string buildChunkResponseMessage(const std::string& file_name, const std::vector<int>& chunks_available) const;
};

#endif // UDPSERVER_H
