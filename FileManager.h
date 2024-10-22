#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <map>
#include <vector>
#include <set>
#include <unordered_map>
#include <mutex>

/**
 * @brief Estrutura que armazena as informações sobre um peer que possui um chunk específico.
 */
struct ChunkLocationInfo {
    std::string ip;          ///< Endereço IP do peer que possui o chunk.
    int port;                ///< Porta do peer que possui o chunk.
    int transfer_speed;      ///< Velocidade de transferência do peer.

    /**
     * @brief Construtor da estrutura ChunkLocationInfo.
     * 
     * Inicializa os membros da estrutura com os valores fornecidos ou, caso não sejam fornecidos, 
     * usa valores padrão.
     * 
     * @param ip Endereço IP do peer que possui o chunk (padrão: string vazia).
     * @param port Porta do peer que possui o chunk (padrão: 0).
     * @param transfer_speed Velocidade de transferência do peer (padrão: 0).
     */
    ChunkLocationInfo(const std::string& ip = "", int port = 0, int transfer_speed = 0)
        : ip(ip), port(port), transfer_speed(transfer_speed) {}
};

/**
 * @brief Classe responsável por gerenciar os arquivos e chunks do peer.
 */
class FileManager {
private:
    std::string peer_id;  
    ///< ID do peer.

    std::map<std::string, std::set<int>> local_chunks;
    ///< Mapa que armazena os chunks locais disponíveis para cada arquivo.
    ///< A chave é o nome do arquivo (std::string).
    ///< O valor é um conjunto (std::set<int>) contendo os IDs dos chunks que o peer já possui para aquele arquivo.

    std::string directory;  
    ///< Diretório onde os arquivos do peer estão armazenados.

    std::unordered_map<std::string, std::vector<std::vector<ChunkLocationInfo>>> chunk_location_info;
    ///< Mapa que armazena informações sobre os peers que possuem cada chunk de um arquivo.
    ///< A chave é o nome do arquivo (std::string).
    ///< O valor é um vetor onde cada índice representa um chunk do arquivo.
    ///< Cada índice contém um vetor de `ChunkLocationInfo`, onde cada `ChunkLocationInfo` descreve um peer
    ///< que possui o chunk, incluindo seu IP, porta UDP e velocidade de transferência.

    std::mutex mapMutex;  
    ///< Mutex usado para garantir acesso seguro ao mapa em ambientes de múltiplas threads.
public:
    /**
     * @brief Construtor da classe FileManager.
     * @param peer_id ID do peer.
     */
    FileManager(const std::string& peer_id);

    /**
     * @brief Carrega os chunks locais disponíveis.
     * Escaneia o diretório do peer e atualiza a lista de chunks que o peer possui.
     */
    void loadLocalChunks();

    /**
     * @brief Verifica se possui um chunk específico de um arquivo.
     * @param file_name Nome do arquivo.
     * @param chunk Número do chunk.
     * @return true se possuir o chunk, false caso contrário.
     */
    bool hasChunk(const std::string& file_name, int chunk);

    /**
     * @brief Retorna o caminho do chunk solicitado.
     * @param file_name Nome do arquivo.
     * @param chunk Número do chunk.
     * @return Caminho completo do chunk.
     */
    std::string getChunkPath(const std::string& file_name, int chunk);

    /**
     * @brief Salva um chunk recebido no diretório do peer.
     * @param file_name Nome do arquivo.
     * @param chunk Número do chunk.
     * @param data Dados do chunk.
     * @param size Tamanho dos dados.
     */
    void saveChunk(const std::string& file_name, int chunk, const char* data, size_t size);

    /**
     * @brief Verifica se todos os chunks de um arquivo foram recebidos.
     * @param file_name Nome do arquivo.
     * @param total_chunks Total de chunks do arquivo.
     * @return true se todos os chunks foram recebidos, false caso contrário.
     */
    bool hasAllChunks(const std::string& file_name, int total_chunks);
    
    /**
     * @brief Retorna os chunks disponíveis para um arquivo específico.
     * @param file_name Nome do arquivo.
     * @return Vetor contendo os chunks disponíveis localmente.
     */
    std::vector<int> getAvailableChunks(const std::string& file_name);

    /**
     * @brief Concatena todos os chunks para formar o arquivo completo.
     * 
     * Combina todos os chunks baixados e monta o arquivo original no diretório do peer.
     * 
     * @param file_name Nome do arquivo.
     * @param total_chunks Total de chunks do arquivo.
     */
    void assembleFile(const std::string& file_name, int total_chunks);

    /**
     * @brief Inicializa a estrutura para armazenar informações sobre onde encontrar cada chunk.
     * 
     * Esta função cria entradas no mapa de respostas para o arquivo especificado,
     * alocando um vetor vazio para cada chunk. Esses vetores serão preenchidos posteriormente
     * com as informações dos peers (IP, porta e velocidade de transferência) que possui
     * aquele chunk específico.
     * 
     * @param file_name O nome do arquivo ao qual o chunk pertence.
     * @param total_chunks O número total de chunks do arquivo que precisam ser inicializados.
     */
    void initializeChunkLocationInfo(const std::string& file_name, int total_chunks);

    /**
     * @brief Armazena informações de chunks recebidos para um arquivo específico.
     * 
     * Insere as informações de um chunk recebido no mapa `chunk_location_info`.
     * Utiliza um mutex para garantir que o acesso ao mapa seja seguro em um ambiente de múltiplas threads.
     * 
     * @param file_name O nome do arquivo associado aos chunks.
     * @param chunk_ids Uma lista de IDs dos chunks que foram recebidos.
     * @param ip O endereço IP do peer que enviou a resposta.
     * @param port A porta do peer que enviou a resposta.
     * @param transfer_speed A velocidade de transferência do peer.
     */
    void storeChunkLocationInfo(const std::string& file_name, const std::vector<int>& chunk_ids, const std::string& ip, int port, int transfer_speed);

    /**
     * @brief Seleciona os peers de onde os chunks serão baixados, mantendo a porta no valor.
     * 
     * Esta função escolhe quais peers (baseados no IP) serão usados para baixar os chunks do arquivo
     * especificado, e retorna um mapa onde a chave é o IP do peer e o valor é um par contendo a porta
     * do peer e a lista de chunks que serão baixados daquele peer.
     * 
     * @param file_name O nome do arquivo cujos chunks serão baixados.
     * @return Mapa onde a chave é o IP do peer e o valor é um par contendo a porta e a lista de chunks a serem baixados daquele peer.
     */
    std::unordered_map<std::string, std::pair<int, std::vector<int>>> FileManager::selectPeersForChunkDownload(const std::string& file_name);

    /**
     * @brief Obtém os chunks ainda não recebidos.
     * 
     * Verifica quais chunks ainda estão pendentes para download, retornando um vetor contendo os IDs desses chunks.
     * 
     * @param file_name Nome do arquivo.
     * @param total_chunks Número total de chunks do arquivo.
     * @return Vetor contendo os IDs dos chunks que ainda precisam ser baixados.
     */
    std::vector<int> getMissingChunks(const std::string& file_name, int total_chunks);
};

// /**
//  * Implementação da função getMissingChunks:
//  * Verifica quais chunks ainda não foram recebidos de um arquivo e retorna um vetor contendo esses chunks.
//  */
// std::vector<int> FileManager::getMissingChunks(const std::string& file_name, int total_chunks) {
//     std::vector<int> missing_chunks;

//     std::lock_guard<std::mutex> lock(mapMutex); // Protege o acesso ao mapa local_chunks

//     for (int chunk = 0; chunk < total_chunks; ++chunk) {
//         if (local_chunks[file_name].find(chunk) == local_chunks[file_name].end()) {
//             // Se o chunk não está disponível localmente, ele é considerado ausente
//             missing_chunks.push_back(chunk);
//         }
//     }
//     return missing_chunks;
// }

#endif // FILEMANAGER_H
