#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <string>
#include <map>
#include <vector>

/**
 * @brief Classe responsável por carregar as configurações dos arquivos topologia.txt e config.txt.
 */
class ConfigManager {
public:
    /**
     * @brief Carrega a topologia da rede a partir do arquivo.
     * @param file_name Nome do arquivo que está a topologia (Ex.: topologia.txt).
     * @return Mapa com a topologia (vizinhos de cada nodo).
     */
    static std::map<int, std::vector<int>> loadTopology(const std::string& file_name);

    /**
     * @brief Carrega as configurações dos peers a partir do arquivo.
     * @param file_name Nome do arquivo que está a configuração (Ex.: config.txt).
     * @return Mapa com as configurações de cada peer.
     */
    static std::map<int, std::tuple<std::string, int, int>> loadConfig(const std::string& file_name);

    
    /**
     * @brief Expande a topologia com as informações detalhadas da configuração dos peers.
     * 
     * @param topology Mapa da topologia da rede.
     * @param config Mapa de configuração dos peers (IP, porta, velocidade).
     * @return std::map<int, std::vector<std::tuple<int, std::string, int, int>>> Topologia detalhada com as informações dos nodos.
     */
    static std::map<int, std::vector<std::tuple<std::string, int>>> expandTopology(
        const std::map<int, std::vector<int>>& topology, 
        const std::map<int, std::tuple<std::string, int, int>>& config
    );
};

#endif // CONFIGMANAGER_H
