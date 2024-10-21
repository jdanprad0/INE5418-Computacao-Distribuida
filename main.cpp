#include "Peer.h"
#include "Utils.h"
#include "ConfigManager.h"
#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <peer_id>" << std::endl;
        return 1;
    }

    // Limpa o terminal antes de iniciar o programa
    system("clear");

    // Mata todos os processos nas portas que serão utilizadas (6000 a 6025 para UDP e 7000 a 7025 para TCP)
    logMessage(LogType::INFO, "Matando processos nas portas 6000-6025 e 7000-7025...");
    system("lsof -ti :6000-6025,7000-7025 | xargs -r kill -9 2>/dev/null");

    int peer_id = std::stoi(argv[1]);

    // Carrega as configurações
    auto topology = ConfigManager::loadTopology("topologia.txt");
    auto config = ConfigManager::loadConfig("config.txt");
    auto expand_topology = ConfigManager::expandTopology(topology, config);

    // Verifica se o peer_id está na configuração
    if (config.find(peer_id) == config.end()) {
        std::cerr << "Peer ID não encontrado nas configurações." << std::endl;
        return 1;
    }

    // Obtém as configurações do peer
    auto [ip, udp_port, speed] = config[peer_id];
    auto neighbors = expand_topology[peer_id];
    int tcp_port = udp_port + 1000; // Exemplo: porta TCP é a UDP + 1000

    // Cria o peer
    Peer peer(peer_id, ip, udp_port, tcp_port, speed, neighbors);

    // Inicia os servidores
    std::thread peer_thread(&Peer::start, &peer);

    // Simula a entrada de um arquivo de metadados para buscar

    if (peer_id == 0) { // gambiarra por enquanto
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Aguarda a inicialização
        peer.searchFile("image.png.p2p");
    }

    peer_thread.join();

    return 0;
}
