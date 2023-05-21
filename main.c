#include "mtwister.h"
#include <limits.h>
#include <openssl/sha.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TOTAL_BLOCOS 1000
#define MAX_TRANSACOES_BLOCO 5
#define MAX_BITCOINS_TRANSACAO 100

struct bloco_nao_minerado {
  uint32_t numero;
  uint32_t nonce;
  unsigned char data[184];
  unsigned char hash_anterior[SHA256_DIGEST_LENGTH];
};

struct bloco_minerado {
  struct bloco_nao_minerado bloco;
  unsigned char hash[SHA256_DIGEST_LENGTH];
  struct bloco_minerado *prox;
};

struct sistema_bitcoin {
  uint32_t carteira[256];
};

void iniciar_carteira(struct sistema_bitcoin *sistema) {
  memset(sistema->carteira, 0, sizeof(sistema->carteira));
}

void gerar_transacoes_bloco(struct bloco_nao_minerado *bloco,
                            unsigned long int endereco_minerador,
                            struct sistema_bitcoin *sistema, MTRand rand) {
  size_t num_transacoes = 0;

  while (num_transacoes < MAX_TRANSACOES_BLOCO) {
    uint32_t enderecos_com_bitcoins[256];
    size_t num_enderecos_com_bitcoins = 0;

    for (uint32_t i = 0; i < 256; i++) {
      if (sistema->carteira[i] > 0) {
        enderecos_com_bitcoins[num_enderecos_com_bitcoins] = i;
        num_enderecos_com_bitcoins++;
      }
    }

    unsigned long int indice_origem =
        genRandLong(&rand) % num_enderecos_com_bitcoins;

    unsigned long int endereco_origem = enderecos_com_bitcoins[indice_origem];
    unsigned long int endereco_destino;

    if (num_enderecos_com_bitcoins == 0 ||
        sistema->carteira[endereco_origem] == 0)
      break;

    do {
      endereco_destino = genRandLong(&rand) % 256;
    } while (endereco_origem == endereco_destino);

    uint32_t saldo_disponivel = sistema->carteira[endereco_origem];
    unsigned long int quantidade = genRandLong(&rand) % (saldo_disponivel + 1);

    if (quantidade > saldo_disponivel)
      quantidade = saldo_disponivel;

    if (quantidade > 0 && sistema->carteira[endereco_origem] >= quantidade) {
      sistema->carteira[endereco_origem] -= quantidade;
      sistema->carteira[endereco_destino] += quantidade;

      // Registrar a transação no bloco
      bloco->data[num_transacoes * 3] = endereco_origem;
      bloco->data[num_transacoes * 3 + 1] = endereco_destino;
      bloco->data[num_transacoes * 3 + 2] = quantidade;

      num_transacoes++;
    }
  }

  bloco->data[183] = (unsigned char)endereco_minerador;
  // printf("DEPOIS DA ATRIBUICAO = %u\n", bloco->data[183]);
}

void minerar_bloco(struct bloco_nao_minerado *bloco, unsigned char *hash) {
  uint32_t nonce = 0;

  unsigned char
      bloco_completo[sizeof(struct bloco_nao_minerado) + sizeof(uint32_t)];

  unsigned char hash_result[SHA256_DIGEST_LENGTH];

  while (nonce < UINT32_MAX) {
    memcpy(bloco_completo, bloco, sizeof(struct bloco_nao_minerado));

    memcpy(bloco_completo + sizeof(struct bloco_nao_minerado), &nonce,
           sizeof(uint32_t));

    SHA256(bloco_completo, sizeof(struct bloco_nao_minerado) + sizeof(uint32_t),
           hash_result);

    if (hash_result[0] == 0 && hash_result[1] == 0) {
      memcpy(hash, hash_result, SHA256_DIGEST_LENGTH);
      bloco->nonce = nonce;
      break;
    }

    nonce++;
  }
}

void inserir_bloco(struct bloco_minerado **blockchain,
                   struct bloco_nao_minerado bloco, unsigned char *hash) {

  // printf("BLOCO RECEBIDO %u\n", bloco.data[183]);
  struct bloco_minerado *novo_bloco =
      (struct bloco_minerado *)malloc(sizeof(struct bloco_minerado));

  memcpy(&(novo_bloco->bloco), &bloco, sizeof(struct bloco_nao_minerado));
  memcpy(novo_bloco->hash, hash, SHA256_DIGEST_LENGTH);
  novo_bloco->prox = NULL;

  if (*blockchain == NULL) {
    *blockchain = novo_bloco;
  } else {
    struct bloco_minerado *atual = *blockchain;
    while (atual->prox != NULL) {
      atual = atual->prox;
    }
    atual->prox = novo_bloco;
  }
}

void imprimir_blocos_minerados(struct bloco_minerado *blockchain,
                               struct sistema_bitcoin sistema) {
  struct bloco_minerado *atual = blockchain;

  while (atual != NULL) {
    printf("Bloco %u:\n", atual->bloco.numero);
    printf("Hash do bloco: ");

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
      printf("%02x", atual->hash[i]);

    printf("\n");
    printf("Hash anterior: ");
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
      printf("%02x", atual->bloco.hash_anterior[i]);
    printf("\n");

    printf("Endereço do minerador: %u\n", atual->bloco.data[183]);

    if (atual->bloco.numero != 1) {

      // printf("\nTransações:\n");

      for (int i = 0; i < MAX_TRANSACOES_BLOCO; i++) {
        uint32_t endereco_origem = atual->bloco.data[i * 3];
        uint32_t endereco_destino = atual->bloco.data[i * 3 + 1];
        uint32_t quantidade = atual->bloco.data[i * 3 + 2];

        printf("Transação %d:\n", i + 1);

        printf("endereco de origem: %u\n", endereco_origem);
        printf("endereco de destino: %u\n", endereco_destino);
        printf("quantidade transferida: %u\n\n", quantidade);
      }
    }
    printf("\n");

    atual = atual->prox;
  }
}

int main() {
  struct sistema_bitcoin sistema;
  iniciar_carteira(&sistema);

  struct bloco_minerado *blockchain = NULL;

  uint32_t numero_bloco;
  unsigned char hash_anterior[SHA256_DIGEST_LENGTH] = {0};

  MTRand rand = seedRand(1234567);
  for (numero_bloco = 1; numero_bloco <= TOTAL_BLOCOS; ++numero_bloco) {
    struct bloco_nao_minerado bloco;
    bloco.numero = numero_bloco;
    memcpy(bloco.hash_anterior, hash_anterior, SHA256_DIGEST_LENGTH);

    unsigned char hash[SHA256_DIGEST_LENGTH];

    if (numero_bloco == 1) {
      const char *mensagem = "The Times 03/Jan/2009 Chancellor on brink of "
                             "second bailout for banks";

      size_t tamanho_mensagem = strlen(mensagem);

      memcpy(bloco.data, mensagem, tamanho_mensagem);
      memset(bloco.data + tamanho_mensagem, 0,
             sizeof(bloco.data) - tamanho_mensagem);

      unsigned long int endereco_minerador = genRandLong(&rand) % 256;
      bloco.data[183] = (unsigned char)endereco_minerador;
      minerar_bloco(&bloco, hash);
      sistema.carteira[(uint32_t)bloco.data[183]] += 50;
    } else {
      unsigned long int endereco_minerador = genRandLong(&rand) % 256;
      gerar_transacoes_bloco(&bloco, endereco_minerador, &sistema, rand);
      minerar_bloco(&bloco, hash);
      sistema.carteira[(uint32_t)bloco.data[183]] += 50;
    }

    inserir_bloco(&blockchain, bloco, hash);

    memcpy(hash_anterior, hash, SHA256_DIGEST_LENGTH);
  }

  imprimir_blocos_minerados(blockchain, sistema);

  struct bloco_minerado *atual = blockchain;

  while (atual != NULL) {
    struct bloco_minerado *proximo = atual->prox;
    free(atual);
    atual = proximo;
  }

  return EXIT_SUCCESS;
}
