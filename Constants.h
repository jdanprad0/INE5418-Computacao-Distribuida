#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

namespace Constants {
    // Caminhos dos arquivos
    const std::string CONFIG_PATH = "./src/config.txt";         ///< Caminho para o arquivo de configuração.
    const std::string TOPOLOGY_PATH = "./src/topologia.txt";    ///< Caminho para o arquivo de topologia.
    const std::string BASE_PATH = "./src/";                     ///< Caminho base onde os arquivos do projeto estão armazenados.

    // Cores para log
    const std::string RESET   = "\033[0m";   ///< Resetar a cor do texto.
    const std::string RED     = "\033[91m";  ///< Cor vermelha.
    const std::string GREEN   = "\033[32m";  ///< Cor verde escura.
    const std::string YELLOW  = "\033[93m";  ///< Cor amarela.
    const std::string MAGENTA = "\033[95m";  ///< Cor magenta.
    const std::string BLUE    = "\033[94m";  ///< Cor azul.
    const std::string CIANO   = "\033[96m";  ///< Cor ciano.
    const std::string VIBRANT_GREEN = "\e[92m";  ///< Cor verde vibrante.

    const int RESPONSE_TIMEOUT_SECONDS = 10; ///< Tempo limite para resposta em segundos.
}

#endif // CONSTANTS_H
