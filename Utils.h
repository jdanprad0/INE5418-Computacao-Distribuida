#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <regex>
#include <iostream>

// Definições de cores
#define RESET   "\033[0m"
#define RED     "\033[91m"
#define GREEN   "\033[92m"
#define YELLOW  "\033[93m"
#define BLUE    "\033[94m"
#define MAGENTA "\033[95m"

/**
 * @brief Remove espaços em branco ao redor de uma string.
 * @param str String que deseja remover os espaços em branco ao redor.
 * @return A string sem espaços em branco ao redor.
 */
std::string trim(const std::string& str);

/**
 * @brief Função auxiliar para formatar e exibir mensagens de log de forma consistente, com cores.
 * 
 * @param type Tipo da mensagem (INFO, ERROR, DISCOVERY, RESPONSE)
 * @param message A mensagem a ser exibida no log
 */
void logMessage(const std::string& type, const std::string& message);

#endif // UTILS_H
