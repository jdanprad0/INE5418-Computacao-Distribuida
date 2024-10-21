#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <string>
#include <map>
#include <vector>
#include <set>
#include <tuple>
#include <unordered_map>
#include <mutex>
#include "FileManager.h"

/**
 * @brief Definição da struct PeerInfo.
 * 
 * Esta struct armazena as informações de um peer, especificamente seu endereço IP
 * e a porta UDP utilizada para comunicação.
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
 * Ela interage com a classe FileManager para verificar e enviar os chunks de arquivos 
 * que o peer possui, bem como para descobrir arquivos na rede.
 */
class UDPServer {
private:
    std::string ip;                                                     ///< Endereço IP do peer atual.
    int port;                                                           ///< Porta UDP que o peer está utilizando para a comunicação.
    int peer_id;                                                        ///< Identificador único (ID) do peer.
    int transfer_speed;                                                 ///< Velocidade de transferência de dados.
    int sockfd;                                                         ///< Descriptor do socket UDP utilizado para a comunicação.
    FileManager& file_manager;                                          ///< Referência ao gerenciador de arquivos (armazenamento e chunks).
    std::vector<std::tuple<std::string, int>> udpNeighbors;             ///< Lista contendo os vizinhos diretos do peer (endereços IP e portas).
    std::map<std::string, bool> processing_active_map;                  ///< Mapa para controlar o estado de processamento de cada arquivo. Mapeia file_name para processing_active.
    std::mutex processing_mutex;                                        ///< Mutex para proteger o acesso ao mapa

public:
    /**
     * @brief Construtor da classe UDPServer.
     * 
     * Inicializa o servidor UDP com o endereço IP, a porta e o ID do peer, e 
     * associa-o a um gerenciador de arquivos (`FileManager`), que é responsável por 
     * armazenar e gerenciar os arquivos e seus chunks.
     * 
     * @param ip Endereço IP do peer.
     * @param port Porta UDP usada para a comunicação.
     * @param peer_id ID do peer na rede P2P.
     * @param transfer_speed Velocidade de transferência de dados do peer na rede P2P.
     * @param file_manager Referência ao gerenciador de arquivos do peer.
     */
    UDPServer(const std::string& ip, int port, int peer_id, int transfer_speed, FileManager& file_manager);

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
     * por peers que estão buscando um arquivo na rede. A função extrai as informações 
     * da mensagem, verifica se o peer atual possui os chunks do arquivo solicitado e, 
     * caso positivo, envia uma resposta. Caso o TTL (Time-to-Live) ainda esteja válido, 
     * a mensagem é propagada para os vizinhos.
     * 
     * @param message Stream com os dados da mensagem DISCOVERY.
     * @param direct_sender_info Informações sobre o peer que enviou diretamente a mensagem, incluindo seu endereço IP e porta UDP.
     */
    void processDiscoveryMessage(std::stringstream& message, const PeerInfo& direct_sender_info);

    /**
     * @brief Processa uma mensagem de resposta (RESPONSE).
     * 
     * Esta função é responsável por processar as respostas recebidas após um peer enviar 
     * uma solicitação de descoberta de arquivo. Ela extrai as informações do peer que 
     * enviou a resposta positiva a sua mensagem de descoberta.
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
     * @param chunk_requester_info Informações sobre o peer que solicitou os chuncks do arquivo, como seu endereço IP e porta UDP.
     */
    void sendDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, const PeerInfo& chunk_requester_info);
    
    /**
     * @brief Envia uma resposta (RESPONSE) contendo os chunks disponíveis para um arquivo.
     * 
     * Após receber uma solicitação de descoberta, essa função envia uma resposta 
     * para o peer solicitante informando quais chunks estão disponíveis.
     * 
     * @param file_name Nome do arquivo solicitado.
     * @param chunk_requester_info Informações sobre o peer que solicitou os chuncks do arquivo, como seu endereço IP e porta UDP.
     * @return Retorna true se possui chunks disponíveis, false caso contrário.
     */
    bool sendChunkResponse(const std::string& file_name, const PeerInfo& chunk_requester_info);

    /**
     * @brief Monta a mensagem de descoberta (DISCOVERY) para envio.
     * 
     * Constrói uma string formatada contendo as informações da mensagem DISCOVERY que será enviada 
     * aos vizinhos para a busca de um arquivo.
     * 
     * @param file_name Nome do arquivo a ser descoberto.
     * @param total_chunks Número total de chunks do arquivo.
     * @param ttl Time-to-live da mensagem DISCOVERY.
     * @param chunk_requester_info Informações sobre o peer que solicitou os chuncks do arquivo, como seu endereço IP e porta UDP.
     * @return String contendo a mensagem DISCOVERY formatada.
     */
    std::string buildDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, const PeerInfo& chunk_requester_info) const;

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

    /**
     * @brief Inicia um timer que desativa o processamento de mensagens RESPONSE após RESPONSE_TIMEOUT_SECONDS segundos.
     * Este método aguarda 5 segundos e então altera o estado de processamento das mensagens
     * RESPONSE para inativo, utilizando um mutex para garantir a segurança em ambientes de 
     * múltiplas threads.
     * @param file_name Nome do arquivo para o qual o timer está sendo iniciado.
     */
    void startTimer(const std::string& file_name);

};

#endif // UDPSERVER_H
