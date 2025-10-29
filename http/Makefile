CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
LDFLAGS = 

# Nomes dos executáveis
CLIENTE = meu_navegador
SERVIDOR = meu_servidor

# Alvo padrão
all: $(CLIENTE) $(SERVIDOR)

# Compilar o cliente
$(CLIENTE): cliente_http.c
	$(CC) $(CFLAGS) -o $(CLIENTE) cliente_http.c $(LDFLAGS)

# Compilar o servidor
$(SERVIDOR): servidor_http.c
	$(CC) $(CFLAGS) -o $(SERVIDOR) servidor_http.c $(LDFLAGS)

# Limpar arquivos compilados
clean:
	rm -f $(CLIENTE) $(SERVIDOR)

# Instalar nos diretórios do sistema (opcional)
install: all
	cp $(CLIENTE) /usr/local/bin/
	cp $(SERVIDOR) /usr/local/bin/

.PHONY: all clean install