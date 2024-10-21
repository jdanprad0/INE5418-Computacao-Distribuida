#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <map>
#include <vector>
#include <set>
#include <unordered_map>
#include <mutex>

struct ChunkResponse {
    std::string ip;          ///< Endereço IP do peer que possui o chunk.
    int port;                ///< Porta do peer que possui o chunk.
    int transfer_speed;      ///< Velocidade de transferência do peer.

    ChunkResponse(const std::string& ip = "", int port = 0, int transfer_speed = 0)
        : ip(ip), port(port), transfer_speed(transfer_speed) {}
};

/**
 * @brief Classe responsável por gerenciar os arquivos e chunks do peer.
 */
class FileManager {
private:
    std::string peer_id;                                                            ///< ID do peer (usado para diretório).
    std::map<std::string, std::set<int>> local_chunks;                              ///< Chunks locais disponíveis.
    std::string directory;                                                          ///< Diretório dos arquivos do peer.
    std::unordered_map<std::string, std::vector<ChunkResponse>> chunk_responses;    ///< Armazena as respostas positivas dos peers: chunk_id -> (ip, port, speed)
    std::mutex mapMutex;                                                            ///< Mutex para proteger o acesso ao mapa

public:
    /**
     * @brief Construtor da classe FileManager.
     * @param peer_id ID do peer.
     */
    FileManager(const std::string& peer_id);

    /**
     * @brief Carrega os chunks locais disponíveis.
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
     * @param file_name Nome do arquivo.
     * @param total_chunks Total de chunks do arquivo.
     */
    void assembleFile(const std::string& file_name, int total_chunks);

    /**
     * @brief Inicializa a estrutura para armazenar as respostas dos chunks.
     * Este método cria placeholders para cada chunk associado ao nome do arquivo
     * no mapa de respostas de chunks.
     * @param file_name O nome do arquivo para o qual os chunks serão armazenados.
     * @param total_chunks O número total de chunks a serem inicializados.
     */
    void initializeChunkResponses(const std::string& file_name, int total_chunks);

    /**
     * @brief Armazena informações de chunks recebidos para um arquivo específico.
     * Esta função insere as informações de um chunk recebido no mapa `chunk_responses`.
     * Utiliza um mutex para garantir que o acesso ao mapa seja seguro em um ambiente de múltiplas threads.
     * @param file_name O nome do arquivo associado aos chunks.
     * @param chunk_ids Uma lista de IDs dos chunks que foram recebidos.
     * @param ip O endereço IP do peer que enviou a resposta.
     * @param port A porta do peer que enviou a resposta.
     * @param transfer_speed A velocidade de transferência do peer.
     */
    void storeChunkResponses(const std::string& file_name, const std::vector<int>& chunk_ids, const std::string& ip, int port, int transfer_speed);
};

#endif // FILEMANAGER_H
