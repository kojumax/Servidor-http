/*
 * Servidor HTTP - Trabalho Acadêmico UFSJ 2025  
 * Departamento de Ciência da Computação
 * Disciplina: Redes e computadores
 * Aluno: André Arcuri Martins - Matrícula: 212050110
 *
 * Implementação de servidor HTTP para servir arquivos
 * Licença MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define PORT 5050
#define BUFFER_SIZE 4096
#define BACKLOG 10

// Função para obter o tipo MIME baseado na extensão do arquivo
const char* obter_tipo_mime(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) return "application/octet-stream";
    
    if (strcasecmp(ext, ".html") == 0 || strcasecmp(ext, ".htm") == 0)
        return "text/html";
    else if (strcasecmp(ext, ".txt") == 0)
        return "text/plain";
    else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    else if (strcasecmp(ext, ".png") == 0)
        return "image/png";
    else if (strcasecmp(ext, ".gif") == 0)
        return "image/gif";
    else if (strcasecmp(ext, ".pdf") == 0)
        return "application/pdf";
    else if (strcasecmp(ext, ".zip") == 0)
        return "application/zip";
    else if (strcasecmp(ext, ".json") == 0)
        return "application/json";
    else
        return "application/octet-stream";
}

// Função para codificar caracteres especiais em HTML
void html_encode(char *dest, const char *src, size_t max_len) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < max_len - 1; i++) {
        switch (src[i]) {
            case '<': 
                if (j + 4 < max_len) { strcpy(dest + j, "&lt;"); j += 4; }
                break;
            case '>': 
                if (j + 4 < max_len) { strcpy(dest + j, "&gt;"); j += 4; }
                break;
            case '&': 
                if (j + 5 < max_len) { strcpy(dest + j, "&amp;"); j += 5; }
                break;
            case '"': 
                if (j + 6 < max_len) { strcpy(dest + j, "&quot;"); j += 6; }
                break;
            default:
                dest[j++] = src[i];
        }
    }
    dest[j] = '\0';
}

// Função para gerar listagem de diretório em HTML
void gerar_listagem_dir(int client_fd, const char *path, const char *request_path) {
    char buffer[BUFFER_SIZE];
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    
    // Cabeçalho HTTP
    char header[1024];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html; charset=utf-8\r\n"
             "Connection: close\r\n"
             "\r\n");
    send(client_fd, header, strlen(header), 0);
    
    // Início do HTML
    char html_header[2048];
    snprintf(html_header, sizeof(html_header),
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head>\n"
             "    <title>Listagem de Diretório: %s</title>\n"
             "    <style>\n"
             "        body { font-family: Arial, sans-serif; margin: 40px; }\n"
             "        h1 { color: #333; }\n"
             "        table { border-collapse: collapse; width: 100%%; margin-top: 20px; }\n"
             "        th, td { border: 1px solid #ddd; padding: 12px; text-align: left; }\n"
             "        th { background-color: #f2f2f2; }\n"
             "        tr:hover { background-color: #f5f5f5; }\n"
             "        a { text-decoration: none; color: #0066cc; }\n"
             "        a:hover { text-decoration: underline; }\n"
             "    </style>\n"
             "</head>\n"
             "<body>\n"
             "    <h1>Listagem de Diretório: %s</h1>\n"
             "    <table>\n"
             "        <tr>\n"
             "            <th>Nome</th>\n"
             "            <th>Tamanho</th>\n"
             "            <th>Modificado</th>\n"
             "        </tr>\n",
             request_path, request_path);
    send(client_fd, html_header, strlen(html_header), 0);
    
    // Abrir diretório
    dir = opendir(path);
    if (dir == NULL) {
        char error_msg[] = "        <tr><td colspan='3'>Erro ao abrir diretório</td></tr>\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
    } else {
        // Entrada para diretório pai (se não for root)
        if (strcmp(request_path, "/") != 0) {
            char parent_dir[1024];
            char encoded_name[1024];
            html_encode(encoded_name, "..", sizeof(encoded_name));
            
            snprintf(parent_dir, sizeof(parent_dir),
                     "        <tr>\n"
                     "            <td><a href='../'>%s</a></td>\n"
                     "            <td>-</td>\n"
                     "            <td>-</td>\n"
                     "        </tr>\n",
                     encoded_name);
            send(client_fd, parent_dir, strlen(parent_dir), 0);
        }
        
        // Listar entradas do diretório
        while ((entry = readdir(dir)) != NULL) {
            // Ignorar diretórios . e ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            
            char full_path[2048];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            
            if (stat(full_path, &file_stat) == 0) {
                char encoded_name[1024];
                html_encode(encoded_name, entry->d_name, sizeof(encoded_name));
                
                char tamanho[64];
                if (S_ISDIR(file_stat.st_mode)) {
                    strcpy(tamanho, "-");
                } else {
                    if (file_stat.st_size < 1024) {
                        snprintf(tamanho, sizeof(tamanho), "%ld bytes", file_stat.st_size);
                    } else if (file_stat.st_size < 1024 * 1024) {
                        snprintf(tamanho, sizeof(tamanho), "%.1f KB", 
                                file_stat.st_size / 1024.0);
                    } else {
                        snprintf(tamanho, sizeof(tamanho), "%.1f MB", 
                                file_stat.st_size / (1024.0 * 1024.0));
                    }
                }
                
                char data_mod[64];
                strftime(data_mod, sizeof(data_mod), "%Y-%m-%d %H:%M:%S", 
                        localtime(&file_stat.st_mtime));
                
                char row[2048];
                snprintf(row, sizeof(row),
                         "        <tr>\n"
                         "            <td><a href='%s%s'>%s%s</a></td>\n"
                         "            <td>%s</td>\n"
                         "            <td>%s</td>\n"
                         "        </tr>\n",
                         entry->d_name, 
                         S_ISDIR(file_stat.st_mode) ? "/" : "",
                         encoded_name,
                         S_ISDIR(file_stat.st_mode) ? "/" : "",
                         tamanho, data_mod);
                send(client_fd, row, strlen(row), 0);
            }
        }
        closedir(dir);
    }
    
    // Fechar HTML
    char html_footer[] = 
             "    </table>\n"
             "</body>\n"
             "</html>\n";
    send(client_fd, html_footer, strlen(html_footer), 0);
}

// Função para servir arquivo
void servir_arquivo(int client_fd, const char *full_path, const char *filename) {
    FILE *file = fopen(full_path, "rb");
    if (file == NULL) {
        char response[] = 
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "404 Not Found";
        send(client_fd, response, strlen(response), 0);
        return;
    }
    
    // Obter tamanho do arquivo
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Enviar cabeçalho HTTP
    char header[1024];
    const char *mime_type = obter_tipo_mime(filename);
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n"
             "\r\n",
             mime_type, file_size);
    send(client_fd, header, strlen(header), 0);
    
    // Enviar conteúdo do arquivo
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_fd, buffer, bytes_read, 0);
    }
    
    fclose(file);
}

// Função para processar requisição
void processar_requisicao(int client_fd, const char *root_dir) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received < 0) {
        perror("recv");
        close(client_fd);
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    // Extrair método e caminho
    char method[16], path[2048];
    if (sscanf(buffer, "%15s %2047s", method, path) != 2) {
        char response[] = 
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "400 Bad Request";
        send(client_fd, response, strlen(response), 0);
        close(client_fd);
        return;
    }
    
    // Só suportamos GET
    if (strcmp(method, "GET") != 0) {
        char response[] = 
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "405 Method Not Allowed";
        send(client_fd, response, strlen(response), 0);
        close(client_fd);
        return;
    }
    
    printf("Requisição: %s %s\n", method, path);
    
    // Construir caminho completo no sistema de arquivos
    char full_path[4096];
    if (strcmp(path, "/") == 0) {
        // Verificar se existe index.html
        snprintf(full_path, sizeof(full_path), "%s/index.html", root_dir);
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            servir_arquivo(client_fd, full_path, "index.html");
        } else {
            // Gerar listagem do diretório
            gerar_listagem_dir(client_fd, root_dir, path);
        }
    } else {
        // Remover barras iniciais para prevenir directory traversal
        const char *clean_path = path;
        while (*clean_path == '/') clean_path++;
        
        snprintf(full_path, sizeof(full_path), "%s/%s", root_dir, clean_path);
        
        // Verificar se é diretório ou arquivo
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Diretório - verificar se tem index.html
                char index_path[4096];
                snprintf(index_path, sizeof(index_path), "%s/index.html", full_path);
                if (stat(index_path, &st) == 0 && S_ISREG(st.st_mode)) {
                    servir_arquivo(client_fd, index_path, "index.html");
                } else {
                    gerar_listagem_dir(client_fd, full_path, path);
                }
            } else if (S_ISREG(st.st_mode)) {
                // Arquivo regular
                servir_arquivo(client_fd, full_path, clean_path);
            } else {
                char response[] = 
                    "HTTP/1.1 403 Forbidden\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "403 Forbidden";
                send(client_fd, response, strlen(response), 0);
            }
        } else {
            char response[] = 
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n"
                "\r\n"
                "404 Not Found";
            send(client_fd, response, strlen(response), 0);
        }
    }
    
    close(client_fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <diretório>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s /home/usuario/meusite\n", argv[0]);
        return 1;
    }
    
    const char *root_dir = argv[1];
    
    // Verificar se diretório existe
    DIR *dir = opendir(root_dir);
    if (dir == NULL) {
        perror("opendir");
        fprintf(stderr, "Erro: Diretório '%s' não existe ou não pode ser acessado\n", root_dir);
        return 1;
    }
    closedir(dir);
    
    // Criar socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }
    
    // Configurar opções do socket
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }
    
    // Configurar endereço
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }
    
    // Listen
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }
    
    printf("Servidor HTTP rodando em http://localhost:%d\n", PORT);
    printf("Servindo arquivos de: %s\n", root_dir);
    printf("Pressione Ctrl+C para parar o servidor\n");
    
    // Loop principal
    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        
        processar_requisicao(client_fd, root_dir);
    }
    
    close(server_fd);
    return 0;
}