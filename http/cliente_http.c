/*
 * Cliente HTTP - Trabalho Acadêmico UFSJ 2025
 * Departamento de Ciência da Computação
 * Disciplina: Redes e computadores
 * Aluno: André Arcuri Martins - Matrícula: 212050110
 * 
 * Implementação de cliente HTTP para download de arquivos
 * Licença MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#define BUFFER_SIZE 4096
#define MAX_REDIRECTS 5

// Estrutura para armazenar informações da URL
typedef struct {
    char protocolo[16];
    char host[256];
    int porta;
    char caminho[1024];
} URLInfo;

// Implementação própria de strncasecmp para portabilidade
int meu_strncasecmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        char c1 = tolower((unsigned char)s1[i]);
        char c2 = tolower((unsigned char)s2[i]);
        if (c1 != c2) return c1 - c2;
        if (c1 == '\0') return 0;
    }
    return 0;
}

// Função para limpar espaços em branco
void trim(char *str) {
    char *end;
    
    // Remover espaços do início
    while(isspace((unsigned char)*str)) str++;
    
    if(*str == 0) return;
    
    // Remover espaços do final
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    
    *(end + 1) = 0;
}

// Função para analisar a URL
int parse_url(const char *url, URLInfo *info) {
    char temp[2048];
    strncpy(temp, url, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    // Inicializar estrutura
    memset(info, 0, sizeof(URLInfo));
    info->porta = 80; // Porta padrão HTTP

    // Verificar se começa com http:// ou https://
    if (strncmp(temp, "http://", 7) == 0) {
        strncpy(info->protocolo, "http", sizeof(info->protocolo) - 1);
        // Pular "http://"
        char *ptr = temp + 7;
        
        // Encontrar fim do host (primeira '/' ou ':')
        char *host_end = strchr(ptr, '/');
        char *port_start = strchr(ptr, ':');
        
        if (host_end == NULL) {
            host_end = ptr + strlen(ptr);
            strcpy(info->caminho, "/");
        } else {
            strncpy(info->caminho, host_end, sizeof(info->caminho) - 1);
            *host_end = '\0';
        }

        // Verificar se há porta especificada
        if (port_start != NULL && port_start < host_end) {
            *port_start = '\0';
            info->porta = atoi(port_start + 1);
        }

        // Copiar host
        strncpy(info->host, ptr, sizeof(info->host) - 1);
        
    } else if (strncmp(temp, "https://", 8) == 0) {
        // Aceitar HTTPS na parse, mas vamos tratar como erro depois
        strncpy(info->protocolo, "https", sizeof(info->protocolo) - 1);
        info->porta = 443; // Porta padrão HTTPS
        
        // Pular "https://"
        char *ptr = temp + 8;
        
        // Encontrar fim do host
        char *host_end = strchr(ptr, '/');
        if (host_end == NULL) {
            host_end = ptr + strlen(ptr);
            strcpy(info->caminho, "/");
        } else {
            strncpy(info->caminho, host_end, sizeof(info->caminho) - 1);
            *host_end = '\0';
        }
        
        // Copiar host
        strncpy(info->host, ptr, sizeof(info->host) - 1);
    } else {
        fprintf(stderr, "Erro: URL deve começar com http:// ou https://\n");
        return -1;
    }

    if (strlen(info->host) == 0) {
        fprintf(stderr, "Erro: Host não especificado na URL\n");
        return -1;
    }

    return 0;
}

// Função para extrair nome do arquivo do caminho
const char* extrair_nome_arquivo(const char *caminho) {
    const char *nome = strrchr(caminho, '/');
    if (nome == NULL || *(nome + 1) == '\0') {
        return "index.html";
    }
    return nome + 1;
}

// Função para criar conexão socket
int criar_conexao(const char *host, int porta) {
    struct hostent *he;
    struct sockaddr_in addr;

    // Obter informação do host
    he = gethostbyname(host);
    if (he == NULL) {
        fprintf(stderr, "Erro gethostbyname: %s\n", hstrerror(h_errno));
        return -1;
    }

    // Criar socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    // Configurar endereço
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(porta);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    // Conectar
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

// Função para enviar requisição HTTP
int enviar_requisicao(int sockfd, const URLInfo *info) {
    char requisicao[2048];
    snprintf(requisicao, sizeof(requisicao),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: MeuNavegador/1.0\r\n"
             "Connection: close\r\n"
             "\r\n",
             info->caminho, info->host);

    int enviado = send(sockfd, requisicao, strlen(requisicao), 0);
    if (enviado <= 0) {
        perror("send");
    }
    return enviado;
}

// Função para processar cabeçalhos HTTP
int processar_cabecalhos(int sockfd, char *buffer, int *tamanho_conteudo, char *localizacao, int *codigo_status) {
    char *fim_linha;
    int cabecalhos_fim = 0;
    *tamanho_conteudo = -1;
    localizacao[0] = '\0';
    *codigo_status = 0;

    // Primeiro, extrair linha de status
    char *status_end = strstr(buffer, "\r\n");
    if (status_end != NULL) {
        *status_end = '\0';
        // Extrair código de status da linha de resposta
        char *codigo_str = strchr(buffer, ' ');
        if (codigo_str != NULL) {
            codigo_str++;
            *codigo_status = atoi(codigo_str);
        }
        memmove(buffer, status_end + 2, strlen(status_end + 2) + 1);
    }

    while (!cabecalhos_fim) {
        // Encontrar fim da linha atual
        fim_linha = strstr(buffer, "\r\n");
        
        if (fim_linha == NULL) {
            // Ler mais dados se necessário
            int lidos = recv(sockfd, buffer + strlen(buffer), 
                           BUFFER_SIZE - strlen(buffer) - 1, 0);
            if (lidos <= 0) break;
            buffer[strlen(buffer) + lidos] = '\0';
            continue;
        }

        *fim_linha = '\0';
        
        // Verificar fim dos cabeçalhos
        if (strlen(buffer) == 0) {
            cabecalhos_fim = 1;
            memmove(buffer, fim_linha + 2, strlen(fim_linha + 2) + 1);
            break;
        }

        // Processar cabeçalhos específicos
        if (meu_strncasecmp(buffer, "Content-Length:", 15) == 0) {
            *tamanho_conteudo = atoi(buffer + 15);
        } else if (meu_strncasecmp(buffer, "Location:", 9) == 0) {
            strncpy(localizacao, buffer + 9, 1023);
            trim(localizacao); // Remover espaços em branco
        }

        // Mover buffer
        memmove(buffer, fim_linha + 2, strlen(fim_linha + 2) + 1);
    }

    return cabecalhos_fim;
}

// Função para salvar arquivo
int salvar_arquivo(const char *nome_arquivo, const char *dados, int tamanho) {
    // Verificar se o nome do arquivo é válido
    if (nome_arquivo == NULL || strlen(nome_arquivo) == 0) {
        nome_arquivo = "downloaded_file";
    }
    
    FILE *arquivo = fopen(nome_arquivo, "wb");
    if (arquivo == NULL) {
        // Tentar com nome alternativo se houver problema
        char alt_nome[256];
        snprintf(alt_nome, sizeof(alt_nome), "download_%ld", (long)time(NULL));
        arquivo = fopen(alt_nome, "wb");
        if (arquivo == NULL) {
            perror("fopen");
            return -1;
        }
        nome_arquivo = alt_nome;
    }

    int escritos = fwrite(dados, 1, tamanho, arquivo);
    fclose(arquivo);

    if (escritos != tamanho) {
        fprintf(stderr, "Erro ao escrever arquivo\n");
        return -1;
    }

    printf("Arquivo '%s' salvo com sucesso (%d bytes)\n", nome_arquivo, tamanho);
    return 0;
}

// Função principal do cliente
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <URL>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s http://www.exemplo.com/arquivo.jpg\n", argv[0]);
        fprintf(stderr, "Exemplo: %s http://localhost:5050/arquivo.pdf\n", argv[0]);
        return 1;
    }

    // Verificar se é HTTPS (não suportado)
    if (strncmp(argv[1], "https://", 8) == 0) {
        fprintf(stderr, "Erro: HTTPS não é suportado. Use HTTP.\n");
        fprintf(stderr, "Tente: http://%s\n", argv[1] + 8);
        return 1;
    }

    URLInfo info;
    int redirect_count = 0;
    char url_atual[2048];
    strncpy(url_atual, argv[1], sizeof(url_atual) - 1);

    while (redirect_count < MAX_REDIRECTS) {
        // Parse da URL
        if (parse_url(url_atual, &info) != 0) {
            return 1;
        }

        // Verificar se é HTTPS (não suportado)
        if (strcmp(info.protocolo, "https") == 0) {
            fprintf(stderr, "Erro: HTTPS não é suportado.\n");
            return 1;
        }

        printf("Conectando a %s:%d...\n", info.host, info.porta);

        // Criar conexão
        int sockfd = criar_conexao(info.host, info.porta);
        if (sockfd < 0) {
            return 1;
        }

        // Enviar requisição
        if (enviar_requisicao(sockfd, &info) <= 0) {
            close(sockfd);
            return 1;
        }

        // Ler resposta
        char buffer[BUFFER_SIZE * 2] = {0};
        int total_lido = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (total_lido <= 0) {
            perror("recv");
            close(sockfd);
            return 1;
        }
        buffer[total_lido] = '\0';

        // Verificar código de status
        int codigo_status;
        int tamanho_conteudo;
        char localizacao[1024];

        processar_cabecalhos(sockfd, buffer, &tamanho_conteudo, localizacao, &codigo_status);

        printf("Status: %d\n", codigo_status);

        // Processar baseado no código de status
        if (codigo_status == 200) {
            // OK - processar conteúdo
            const char *nome_arquivo = extrair_nome_arquivo(info.caminho);
            
            // Se não sabemos o tamanho, ler até o fim da conexão
            if (tamanho_conteudo == -1) {
                // Já temos alguns dados no buffer
                int dados_no_buffer = strlen(buffer);
                char *conteudo_completo = malloc(dados_no_buffer + BUFFER_SIZE);
                if (conteudo_completo == NULL) {
                    perror("malloc");
                    close(sockfd);
                    return 1;
                }
                memcpy(conteudo_completo, buffer, dados_no_buffer);
                int total_dados = dados_no_buffer;

                // Ler resto dos dados
                int lidos;
                while ((lidos = recv(sockfd, conteudo_completo + total_dados, BUFFER_SIZE, 0)) > 0) {
                    total_dados += lidos;
                    char *temp = realloc(conteudo_completo, total_dados + BUFFER_SIZE);
                    if (temp == NULL) {
                        free(conteudo_completo);
                        close(sockfd);
                        return 1;
                    }
                    conteudo_completo = temp;
                }

                salvar_arquivo(nome_arquivo, conteudo_completo, total_dados);
                free(conteudo_completo);
            } else {
                // Sabemos o tamanho exato
                int ja_lido = strlen(buffer);
                salvar_arquivo(nome_arquivo, buffer, ja_lido);
                
                // Ler resto se necessário
                int restante = tamanho_conteudo - ja_lido;
                
                if (restante > 0) {
                    FILE *arquivo = fopen(nome_arquivo, "ab");
                    if (arquivo == NULL) {
                        perror("fopen");
                        close(sockfd);
                        return 1;
                    }
                    
                    while (restante > 0) {
                        int lidos = recv(sockfd, buffer, 
                                      (restante < BUFFER_SIZE) ? restante : BUFFER_SIZE, 0);
                        if (lidos <= 0) break;
                        fwrite(buffer, 1, lidos, arquivo);
                        restante -= lidos;
                    }
                    fclose(arquivo);
                }
            }

            close(sockfd);
            break;

        } else if (codigo_status == 301 || codigo_status == 302) {
            // Redirecionamento
            close(sockfd);

            if (localizacao[0] == '\0') {
                fprintf(stderr, "Redirecionamento sem localização\n");
                return 1;
            }

            printf("Redirecionando para: %s\n", localizacao);
            
            // Verificar se é redirecionamento para HTTPS
            if (strncmp(localizacao, "https://", 8) == 0) {
                fprintf(stderr, "Erro: Redirecionamento para HTTPS não suportado: %s\n", localizacao);
                fprintf(stderr, "Dica: Tente usar a URL HTTPS diretamente (quando implementado)\n");
                return 1;
            }
            
            // Verificar se é redirecionamento absoluto
            if (strncmp(localizacao, "http://", 7) == 0) {
                strncpy(url_atual, localizacao, sizeof(url_atual) - 1);
            } else {
                // Redirecionamento relativo - construir URL completa
                snprintf(url_atual, sizeof(url_atual), "http://%s%s", info.host, localizacao);
            }
            
            redirect_count++;

        } else {
            // Outro código de erro
            fprintf(stderr, "Erro HTTP: %d\n", codigo_status);
            
            // Mostrar corpo da mensagem de erro se disponível
            if (strlen(buffer) > 0) {
                printf("Mensagem: %s\n", buffer);
            }
            
            close(sockfd);
            return 1;
        }
    }

    if (redirect_count >= MAX_REDIRECTS) {
        fprintf(stderr, "Máximo de redirecionamentos excedido\n");
        return 1;
    }

    return 0;
}