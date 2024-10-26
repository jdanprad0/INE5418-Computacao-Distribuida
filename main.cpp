#include "ConfigManager.h"
#include "Peer.h"
#include "Utils.h"
#include <iostream>
#include <thread>


int main(int argc, char* argv[]) {
    if (argc < 2) {
        logMessage(LogType::ERROR, "Uso: " + std::string(argv[0]) + " <peer_id>");
        return 1;
    }

    // Limpa o terminal antes de iniciar o programa
    system("clear");

    // Configuração para flush automático após cada operação de saída
    std::cout.setf(std::ios::unitbuf);

    // Identifica o Peer
    int peer_id = std::stoi(argv[1]);
    logMessage(LogType::INFO, "Peer " + std::to_string(peer_id) + " inicializado.");
    
    // Carrega as configurações
    auto config = ConfigManager::loadConfig();

    // Verifica se o peer_id está na configuração
    if (config.find(peer_id) == config.end()) {
        logMessage(LogType::ERROR, "Peer " + std::to_string(peer_id) + " não encontrado nas configurações.");
        return 1;
    }

    // Obtém as configurações do peer
    auto [ip, udp_port, speed] = config[peer_id];
    int tcp_port = udp_port + 1000; // Exemplo: porta TCP é a UDP + 1000

    // Mata os processos nas portas que serão utilizadas para comunicação TCP e UDP
    system(("lsof -ti :" + std::to_string(tcp_port) + "," + std::to_string(udp_port) + " | xargs -r kill -9 2>/dev/null").c_str());
    // Pequeno atraso para esperar a liberação das portas
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Carrega a topologia
    auto topology = ConfigManager::loadTopology();

    // Verifica se o peer_id está na configuração
    if (topology.find(peer_id) == topology.end()) {
        logMessage(LogType::ERROR, "Peer " + std::to_string(peer_id) + " não encontrado na topologia.");
        return 1;
    }

    //  Expande ela para incluir informações dos vizinhos do peer
    auto expand_topology = ConfigManager::expandTopology(topology, config);

    // Pega os vizinhos do peer
    auto neighbors = expand_topology[peer_id];
    
    // Cria o peer
    Peer peer(peer_id, ip, udp_port, tcp_port, speed, neighbors);

    // Inicia o Peer
    peer.start();

    return 0;
}
