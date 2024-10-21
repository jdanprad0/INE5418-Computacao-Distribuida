#!/bin/bash

# Acessa o diretório do projeto
cd /home/ubuntu/INE5418-05208-20242-Computacao-Distribuida || exit

# Faz o git pull
git pull origin main

# Executa make clean e make
make clean
make

# Define o arquivo de configuração
config_file="src/config.txt"

# Obtém o IP público da máquina
public_ip=$(curl -s ifconfig.me)

# Inicializa a variável que irá armazenar o número correspondente ao IP
index=""

# Lê o arquivo de configuração e encontra o índice correspondente ao IP
while IFS= read -r line; do
    ip=$(echo "$line" | cut -d ',' -f 1 | cut -d ' ' -f 2) # Extrai o IP da linha
    if [[ "$ip" == "$public_ip" ]]; then
        index=$(echo "$line" | cut -d ':' -f 1) # Extrai o índice da linha
        break
    fi
done < "$config_file"

# Verifica se o índice foi encontrado
if [[ -n "$index" ]]; then
    echo "Executando ./p2p com o índice $index..."
    ./p2p "$index" # Chama o programa com o índice correspondente
else
    echo "IP público $public_ip não encontrado no arquivo de configuração."
fi
#