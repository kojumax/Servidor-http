# Servidor-http
Descrição do Trabalho
Parte 1 - Cliente HTTP
Criar um cliente HTTP (navegador) que acesse sites e salve os arquivos recebidos.

Parte 2 - Servidor HTTP
Criar um servidor HTTP que sirva arquivos de um determinado diretório, com listagem automática quando não houver index.html.

Compilação
bash

# Compilar ambos cliente e servidor
make

# Compilar apenas o cliente
make meu_navegador

# Compilar apenas o servidor
make meu_servidor

# Limpar arquivos compilados
make clean

Como Usar
Servidor HTTP
bash

# Servir arquivos de um diretório
./meu_servidor /caminho/para/diretorio

# Exemplo: servir arquivos do diretório atual
./meu_servidor .

# O servidor estará disponível em: http://localhost:5050
Cliente HTTP
bash

# Baixar arquivo de um servidor
./meu_navegador http://localhost:5050/arquivo.txt

# Baixar página principal
./meu_navegador http://localhost:5050/

# Exemplo com site externo
./meu_navegador http://www.exemplo.com/imagem.jpg


# Iniciar servidor
./meu_servidor .

# Testar diferentes URLs em outro terminal:
./meu_navegador http://localhost:5050/
./meu_navegador http://localhost:5050/index.html
./meu_navegador http://localhost:5050/exemplo.txt
./meu_navegador http://localhost:5050/secreto.pdf

Teste 3: Listagem de Diretório
bash

Funcionalidades Implementadas
Cliente HTTP (meu_navegador)

    Requisições GET HTTP/1.1

    Suporte a redirecionamentos (301, 302)

    Download e salvamento de arquivos

    Parsing de URLs

    Tratamento de cabeçalhos HTTP

    Suporte a diferentes tipos de conteúdo

Servidor HTTP (meu_servidor)

    Servir arquivos estáticos

    Listagem automática de diretórios

    Suporte a arquivo index.html

    Interface HTML para listagens

    Códigos de status HTTP (200 e 404)

Estrutura do Projeto
raiz/
├── Makefile              # Script de compilação
├── README.md             # Este arquivo
├── LICENSE               # Licença MIT
├── cliente_http.c        # Implementação do cliente
├── servidor_http.c       # Implementação do servidor
└── exemplos/             # Exemplos para teste
    └── index.html        # Página de exemplo

Como o Código Funciona
Cliente HTTP

    Parsing da URL: Divide a URL em protocolo, host, porta e caminho

    Conexão TCP: Estabelece conexão com o servidor na porta 80 (HTTP)

    Envio de Requisição: Envia requisição GET com cabeçalhos HTTP

    Processamento de Resposta:

        Lê linha de status HTTP

        Processa cabeçalhos (Content-Length, Location, etc.)

        Salva conteúdo no arquivo

    Tratamento de Redirecionamentos: Segue redirecionamentos 301/302

Servidor HTTP

    Inicialização: Cria socket e escuta na porta 5050

    Accept de Conexões: Aceita conexões de clientes

    Parsing de Requisição: Extrai método e caminho da requisição

    Servir Conteúdo:

        Se existe index.html: serve o arquivo

        Se é diretório sem index.html: gera listagem HTML

        Se é arquivo: serve com tipo MIME apropriado

    Geração de Resposta: Envia cabeçalhos HTTP + conteúdo

Limitações Conhecidas

    Não suporta HTTPS - apenas HTTP

    Apenas método GET - outros métodos não implementados

    Redirecionamentos relativos podem não funcionar corretamente

    Sem persistência de conexão - sempre usa "Connection: close"

    Sem cache - sempre baixa arquivos novamente


 Exemplo de Saída
Cliente:
text

Conectando a localhost:5050...
Status: 200
Arquivo 'index.html' salvo com sucesso (127 bytes)

Servidor:
text

Servidor HTTP rodando em http://localhost:5050
Servindo arquivos de: /home/usuario/meusite
Requisição: GET /
Requisição: GET /style.css

 Licença

MIT License - veja arquivo LICENSE para detalhes.
 Objetivo do Trabalho

Este projeto demonstra o funcionamento básico do protocolo HTTP através da implementação prática de um cliente e servidor, mostrando como navegadores e servidores web se comunicam na internet.
