#include "mtwister.h"
#include <assert.h>
#include <limits.h>
#include <openssl/sha.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define TOTAL_BLOCOS 30000
#define MAX_TRANSACOES_BLOCO 61
#define NUM_ENDERECOS 256
#define DATA_LENGTH MAX_TRANSACOES_BLOCO * 3 + 1
#define RECOMPENSA_MINERACAO 50

#define MAX_BLOCOS_BUFFER 16

/* Para criar o arquivo de indice eu usei uma struct de tamanho 16bytes, logo
 * 4096 / 16 = 256. Assim eu envio um buffer de 256 posicoes de uma vez só */
#define MAX_ENDERECO_BUFFER 256

/* Mesma logica do define acima. Eu ja tenho defines com valores 256 mas nao
 * posso usar eles ao longo do programa, pois se um define mudar automaticamente
 * o calculo dos sizeofs pode ser afetados, por isso tem defines com valores
 * repetidos */
#define MAX_NONCE_BUFFER 256

/* Apenas para manter o controle de quem é o bloco genesis */
enum { BLOCO_GENESIS = 1 };

struct bloco_nao_minerado {
  uint32_t numero;
  uint32_t nonce;
  unsigned char data[DATA_LENGTH];
  unsigned char hash_anterior[SHA256_DIGEST_LENGTH];
};

struct bloco_minerado {
  struct bloco_nao_minerado bloco;
  unsigned char hash[SHA256_DIGEST_LENGTH];
  struct bloco_minerado *prox;
};

/* aqui vai estar armazenado a maior e menor transacao de um determinado bloco,
 * na minha lista eu faço a contagem de transacao de cada bloco e verifico o
 * hash do bloco com base nesse campos */
struct estatisticas {
  unsigned long int maior_transacao;
  unsigned long int menor_transacao;

  unsigned long int quantidade_minerados;

  /* Manter controle do tamanho da lista que possui enderecos com bitcoin, assim
   * nao preciso de uma funcao para percorrer a lista toda hora */
  int tamanho_lista_enderecos_com_bitcoin;

  float quantidade_total_bitcoins_por_bloco;

  /* Sempre que um endereco minera um bloco eu incremento na posicao
   * correspondente do array */
  int array_quantidade_de_blocos_minerados[NUM_ENDERECOS];
};

struct sistema_bitcoin {
  uint32_t carteira[NUM_ENDERECOS];
};

struct enderecos_bitcoin {
  uint8_t chave;
  struct enderecos_bitcoin *prox;
};

/* Struct de 16bytes utilizada para criar o arquivo de indice do minerador */
struct indice_registro {
  uint8_t endereco;
  long offset;
};

/*  Struct 16 bytes para criar o arquivo de indice do nonce */
struct indice_nonce {
  uint32_t nonce;
  long offset;
};

struct transacao_ordenada {
  int quantidade;
  struct bloco_minerado *bloco;
};

void insere_arquivo(struct bloco_minerado **blockchain,
                    const char *nomeArquivo);

struct bloco_minerado *le_arquivo(const char *nomeArquivo);

void criar_arquivo_indice(const char *nome_arquivo,
                          const char *nome_arquivo_indice);

void criar_arquivo_indice_nonce(const char *nome_arquivo,
                                const char *nome_arquivo_indice);

void exibir_dados_bloco(struct bloco_minerado bloco);

void imprimir_blocos_nonce(const char *nome_arquivo,
                           const char *nome_arquivo_indice);

void imprimir_blocos_endereco(const char *nome_arquivo,
                              const char *nome_arquivo_indice);

void imprimir_arquivo_indice(const char *nomeArquivoIndice);
void imprimir_bloco_arquivo_binario(const char *nome_arquivo);
void iniciar_carteira(struct sistema_bitcoin *sistema);
void handle_file(FILE **file, const char *filename, const char *write_mode);

void inserir_enderecos_com_bitcoins(struct enderecos_bitcoin **raiz,
                                    struct estatisticas **est, uint32_t chave);

void verificar_hash_com_menos_transacao(struct bloco_minerado *blockchain,
                                        struct estatisticas *est);

void verificar_hash_com_mais_transacao(struct bloco_minerado *blockchain,
                                       struct estatisticas *est);

void encontrar_maior_numero_bitcoins(uint32_t carteira[]);
int escolhe_carteira(struct enderecos_bitcoin *raiz, uint32_t indice);

void minerar_bloco(struct bloco_nao_minerado *bloco, unsigned char *hash,
                   struct estatisticas **est);

void atualizar_maior_menor_transacao(struct estatisticas *est,
                                     unsigned long int num_transacoes);

void endereco_com_mais_blocos_minerados(struct estatisticas est);
void quantidade_media_bitcoins_bloco(struct estatisticas est);

void remove_no(struct enderecos_bitcoin **raiz, struct estatisticas **est,
               uint32_t chave);

int checa_zero_bitcoin(struct enderecos_bitcoin **raiz,
                       struct estatisticas **est,
                       struct sistema_bitcoin *sistema);

void imprime_lista(struct enderecos_bitcoin *raiz);

void gerar_transacoes_bloco(struct bloco_nao_minerado *bloco,
                            struct sistema_bitcoin *sistema, MTRand *rand,
                            struct enderecos_bitcoin **raiz,
                            struct estatisticas **est, unsigned char *hash);

void inserir_bloco(struct bloco_minerado **blockchain,
                   struct bloco_nao_minerado *bloco, unsigned char *hash);

void imprimir_blocos_minerados(struct bloco_minerado *blockchain,
                               const char *filename);

void ordernar_vetor(struct transacao_ordenada *arr, int n);
void transacoes_crescente(struct bloco_minerado *blockchain);
void free_blockchain(struct bloco_minerado **blockchain);

void processar_bloco(struct enderecos_bitcoin **raiz,
                     struct bloco_minerado **blockchain,
                     struct sistema_bitcoin *sistema, struct estatisticas *est);

void limpa_buffer(void);
