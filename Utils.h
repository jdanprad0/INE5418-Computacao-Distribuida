#ifndef UTILS_H
#define UTILS_H

#include "Constants.h"
#include <string>
#include <regex>
#include <iostream>

// Enumeração para os tipos de mensagens de log
enum class LogType {
    ERROR,
    INFO,
    DISCOVERY_RECEIVED,
    DISCOVERY_SENT,
    REQUEST_RECEIVED,
    REQUEST_SENT,
    RESPONSE_RECEIVED,
    RESPONSE_SENT,
    SUCCESS,
    OTHER // Para outros tipos não especificados
};

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
void logMessage(LogType type, const std::string& message);

/**
 * @brief Cria e configura uma estrutura sockaddr_in com base no IP e na porta fornecidos.
 * 
 * @param ip Endereço IP a ser configurado.
 * @param port Porta a ser configurada.
 * @return struct sockaddr_in Estrutura sockaddr_in configurada.
 */
struct sockaddr_in createSockAddr(const std::string& ip, int port);

#endif // UTILS_H
