#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "Utils.h"
#include "Constants.h"
#include <string>
#include <map>
#include <vector>

/**
 * @brief Classe responsável por carregar as configurações dos arquivos topologia.txt e config.txt.
 * 
 * Esta classe fornece métodos estáticos para carregar a topologia da rede e as configurações
 * dos peers a partir de arquivos. A topologia é representada como um mapa que mapeia cada 
 * nodo a seus vizinhos, enquanto as configurações incluem informações como IP, porta UDP e
 * velocidade de transferência em bytes/segundo para cada peer.
 */
class ConfigManager {
public:
    /**
     * @brief Carrega a topologia da rede a partir do arquivo.
     * 
     * Este método lê um arquivo de topologia e constrói um mapa onde cada chave é o identificador 
     * de um nodo e o valor é um vetor contendo os identificadores dos vizinhos desse nodo.
     * 
     * @return Mapa com a topologia (vizinhos de cada nodo).
     */
    static std::map<int, std::vector<int>> loadTopology();

    /**
     * @brief Carrega as configurações dos peers a partir do arquivo.
     * 
     * Este método lê um arquivo de configuração e retorna um mapa onde cada chave é o identificador 
     * de um peer e o valor é uma tupla contendo o IP, porta UDP e velocidade de transferência em bytes/segundo desse peer.
     * 
     * @return Mapa com as configurações de cada peer.
     */
    static std::map<int, std::tuple<std::string, int, int>> loadConfig();

    /**
     * @brief Expande a topologia com as informações detalhadas da configuração dos peers.
     * 
     * Este método combina a topologia da rede com as informações de configuração de cada peer, 
     * criando um mapa que associa cada nodo a uma lista de tuplas, onde cada tupla contém o IP 
     * e a porta dos vizinhos do nodo.
     * 
     * @param topology Mapa da topologia da rede.
     * @param config Mapa de configuração dos peers (IP, porta UDP, velocidade em bytes/segundo).
     * @return std::map<int, std::vector<std::tuple<std::string, int>>> Topologia detalhada com as informações dos nodos.
     */
    static std::map<int, std::vector<std::tuple<std::string, int>>> expandTopology(
        const std::map<int, std::vector<int>>& topology, 
        const std::map<int, std::tuple<std::string, int, int>>& config
    );
};

#endif // CONFIGMANAGER_H
