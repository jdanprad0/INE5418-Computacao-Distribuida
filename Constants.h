#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

namespace Constants {
    // Caminhos dos arquivos
    const std::string CONFIG_PATH = "./src/config.txt";         ///< Caminho para o arquivo de configuração.
    const std::string TOPOLOGY_PATH = "./src/topologia.txt";    ///< Caminho para o arquivo de topologia.
    const std::string BASE_PATH = "./src/";                     ///< Caminho base onde os arquivos do projeto estão armazenados.

    // Cores para log
    const std::string RESET   = "\033[0m";          ///< Resetar a cor do texto.
    const std::string RED     = "\033[91m";         ///< Cor vermelha.
    const std::string GREEN   = "\e[92m";           ///< Cor verde.
    const std::string YELLOW  = "\033[93m";         ///< Cor amarela.
    const std::string MAGENTA = "\033[95m";         ///< Cor magenta.
    const std::string BLUE    = "\033[94m";         ///< Cor azul.
    const std::string CIANO   = "\033[96m";         ///< Cor ciano.
    const std::string GRAY    = "\033[37m";         ///< Cor cinza claro.
    const std::string ORANGE  = "\033[38;5;208m";   ///< Cor laranja.
    const std::string PINK    = "\033[38;5;213m";   ///< Cor rosa 
    const std::string PURPLE  = "\033[38;5;177m";   ///< Cor roxa.
    const std::string GOLD    = "\033[38;5;220m";   ///< Cor dourado.
    const std::string AQUA    = "\033[38;5;51m";    ///< Cor aqua (ciano claro).

    const int RESPONSE_TIMEOUT_SECONDS     = 10;    ///< Tempo limite para resposta em segundos.
    const int TCP_CONTROL_MESSAGE_MAX_SIZE = 1024;  ///< Tamanho máximo de da mensagem de controle.
    const int TCP_MAX_PENDING_CONNECTIONS  = 10;    ///< Número máximo de conexões pendentes na fila de escuta TCP.
}

#endif // CONSTANTS_H
