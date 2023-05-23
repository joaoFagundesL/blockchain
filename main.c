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

#define TOTAL_BLOCOS 5
#define MAX_TRANSACOES_BLOCO 5
#define MAX_BITCOINS_TRANSACAO 100
#define NUM_ENDERECOS 256
#define DATA_LENGTH 184
#define RECOMPENSA_MINERACAO 50

struct bloco_nao_minerado
{
  uint32_t numero;
  uint32_t nonce;
  unsigned char data[DATA_LENGTH];
  unsigned char hash_anterior[SHA256_DIGEST_LENGTH];
};

struct bloco_minerado
{
  struct bloco_nao_minerado bloco;
  unsigned char hash[SHA256_DIGEST_LENGTH];
  struct bloco_minerado *prox;
};

struct sistema_bitcoin
{
  uint32_t carteira[NUM_ENDERECOS];
};

struct enderecos_bitcoin
{
  uint8_t chave;
  struct enderecos_bitcoin *prox;
};

void iniciar_carteira(struct sistema_bitcoin *sistema)
{
  memset(sistema->carteira, 0, sizeof(sistema->carteira));
}

void inserir_enderecos_com_bitcoins(struct enderecos_bitcoin **raiz,
                                    uint8_t chave)
{
  struct enderecos_bitcoin *novo =
      (struct enderecos_bitcoin *)malloc(sizeof(struct enderecos_bitcoin));

  assert(novo != NULL);

  novo->chave = chave;
  novo->prox = *raiz;
  *raiz = novo;
}

void encontrar_maior_numero_bitcoins(uint32_t carteira[])
{
  int num_enderecos_max_bitcoins = 0;
  uint32_t max_bitcoins = 0;

  for (size_t i = 0; i < NUM_ENDERECOS; i++)
  {
    if (carteira[i] > max_bitcoins)
    {
      max_bitcoins = carteira[i];
    }
  }

  uint8_t *enderecos_max_bitcoins = malloc(sizeof(uint8_t));
  assert(enderecos_max_bitcoins != NULL);
  for (size_t i = 0; i < NUM_ENDERECOS; i++)
  {
    if (carteira[i] == max_bitcoins)
    {
      enderecos_max_bitcoins[num_enderecos_max_bitcoins] = i;
      num_enderecos_max_bitcoins++;

      enderecos_max_bitcoins =
          realloc(enderecos_max_bitcoins,
                  sizeof(uint8_t) * (num_enderecos_max_bitcoins + 1));
      assert(enderecos_max_bitcoins != NULL);
    }
  }

  printf("Maior numero de bitcoins = %u nos seguintes enderecos\n",
         max_bitcoins);
  for (size_t i = 0; i < num_enderecos_max_bitcoins; i++)
  {
    printf("%u\n", enderecos_max_bitcoins[i]);
  }
}

int escolhe_carteira(struct enderecos_bitcoin *raiz, uint8_t indice)
{
  size_t i = 0;
  if (!raiz)
    return 0;
  else if (raiz)
  {
    while (i != indice && raiz != NULL)
    {
      raiz = raiz->prox;
      i++;
    }
    return raiz->chave;
  }
  else
    return 0;
}

int conta_enderecos(struct enderecos_bitcoin *raiz)
{
  size_t cont = 0;
  if (!raiz)
    return 0;
  if (raiz)
    while (raiz)
    {
      raiz = raiz->prox;
      cont++;
    }
  return cont;
}

void gerar_transacoes_bloco(struct bloco_nao_minerado *bloco,
                            unsigned long int endereco_minerador,
                            struct sistema_bitcoin *sistema, MTRand rand,
                            struct enderecos_bitcoin **raiz)
{
  size_t num_transacoes = 0;
  uint32_t carteira_auxiliar[NUM_ENDERECOS] = {0};

  while (num_transacoes < MAX_TRANSACOES_BLOCO)
  {
    size_t num_enderecos_com_bitcoins = 0;

    int valor_lista_origem = genRandLong(&rand) % (conta_enderecos(*raiz));

    unsigned long int endereco_origem =
        escolhe_carteira(*raiz, valor_lista_origem);

    unsigned long int endereco_destino;

    do
    {
      endereco_destino = genRandLong(&rand) % NUM_ENDERECOS;
    } while (endereco_origem == endereco_destino);

    uint32_t saldo_disponivel = sistema->carteira[endereco_origem];
    unsigned long int quantidade = genRandLong(&rand) % (saldo_disponivel + 1);

    sistema->carteira[endereco_origem] -= quantidade;
    carteira_auxiliar[endereco_destino] += quantidade;

    bloco->data[num_transacoes * 3] = endereco_origem;
    bloco->data[num_transacoes * 3 + 1] = endereco_destino;
    bloco->data[num_transacoes * 3 + 2] = quantidade;

    num_transacoes++;
  }

  bloco->data[DATA_LENGTH - 1] = (unsigned char)endereco_minerador;

  for (size_t i = 0; i < NUM_ENDERECOS; i++)
  {
    if (carteira_auxiliar[i] > 0)
    {
      sistema->carteira[i] += carteira_auxiliar[i];
      inserir_enderecos_com_bitcoins(raiz, i);
    }
  }
}

void minerar_bloco(struct bloco_nao_minerado *bloco, unsigned char *hash)
{
  uint32_t nonce = 0;

  unsigned char
      bloco_completo[sizeof(struct bloco_nao_minerado) + sizeof(uint32_t)];

  unsigned char hash_result[SHA256_DIGEST_LENGTH];

  while (nonce < UINT32_MAX)
  {
    memcpy(bloco_completo, bloco, sizeof(struct bloco_nao_minerado));

    memcpy(bloco_completo + sizeof(struct bloco_nao_minerado), &nonce,
           sizeof(uint32_t));

    SHA256(bloco_completo, sizeof(struct bloco_nao_minerado) + sizeof(uint32_t),
           hash_result);

    if (hash_result[0] == 0 && hash_result[1] == 0)
    {
      memcpy(hash, hash_result, SHA256_DIGEST_LENGTH);
      bloco->nonce = nonce;
      break;
    }

    nonce++;
  }
}

void inserir_bloco(struct bloco_minerado **blockchain,
                   struct bloco_nao_minerado bloco, unsigned char *hash)
{

  struct bloco_minerado *novo_bloco =
      (struct bloco_minerado *)malloc(sizeof(struct bloco_minerado));

  assert(novo_bloco != NULL);

  memcpy(&(novo_bloco->bloco), &bloco, sizeof(struct bloco_nao_minerado));
  memcpy(novo_bloco->hash, hash, SHA256_DIGEST_LENGTH);
  novo_bloco->prox = NULL;

  if (*blockchain == NULL)
  {
    *blockchain = novo_bloco;
  }
  else
  {
    struct bloco_minerado *atual = *blockchain;
    while (atual->prox != NULL)
    {
      atual = atual->prox;
    }
    atual->prox = novo_bloco;
  }
}

void imprimir_blocos_minerados(struct bloco_minerado *blockchain,
                               struct sistema_bitcoin sistema)
{
  struct bloco_minerado *atual = blockchain;

  while (atual != NULL)
  {
    printf("Bloco %u:\n", atual->bloco.numero);
    printf("Hash do bloco: ");

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
      printf("%02x", atual->hash[i]);

    printf("\n");
    printf("Hash anterior: ");
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
      printf("%02x", atual->bloco.hash_anterior[i]);
    printf("\n");

    printf("Endereço do minerador: %u\n", atual->bloco.data[DATA_LENGTH - 1]);

    if (atual->bloco.numero != 1)
    {

      for (int i = 0; i < MAX_TRANSACOES_BLOCO; i++)
      {
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

void remove_no(struct enderecos_bitcoin **raiz, uint8_t chave)
{
  if (*raiz == NULL)
    return;

  if ((*raiz)->chave == chave)
  {
    struct enderecos_bitcoin *temp = *raiz;

    *raiz = (*raiz)->prox;

    free(temp);

    temp = NULL;

    return;
  }

  struct enderecos_bitcoin *atual = *raiz;
  struct enderecos_bitcoin *anterior = NULL;

  while (atual != NULL && atual->chave != chave)
  {
    anterior = atual;
    atual = atual->prox;
  }

  if (atual == NULL)
    return;
  anterior->prox = atual->prox;
  free(atual);
}

int checa_zero_bitcoin(struct enderecos_bitcoin **raiz,
                       struct sistema_bitcoin *sistema)
{

  if (*raiz == NULL)
    return 1;
  struct enderecos_bitcoin *atual = *raiz;

  while (atual)
  {

    if (sistema->carteira[atual->chave] <= 0)
    {
      // printf("Remoção do %d\n", atual->chave);
      // printf("Valor na carteira: %d\n", sistema->carteira[140]);
      remove_no(raiz, atual->chave);
      return 0;
    }
    atual = atual->prox;
  }
  return 1;
}

void imprime_lista(struct enderecos_bitcoin *raiz)
{
  while (raiz != NULL)
  {
    printf("%d ",
           raiz->chave); // Supondo que o campo a ser impresso seja 'valor'
    raiz = raiz->prox;
  }

  printf("\n");
}

void free_blockchain(struct bloco_minerado **blockchain)
{
  while (*blockchain != NULL)
  {
    struct bloco_minerado *proximo = (*blockchain)->prox;
    free(*blockchain);
    *blockchain = proximo;
  }
}

void processar_bloco(struct enderecos_bitcoin **raiz,
                     struct bloco_minerado **blockchain,
                     struct sistema_bitcoin *sistema)
{

  struct bloco_nao_minerado bloco;

  uint8_t FLAG_CHECA_CARTEIRAS;

  size_t numero_bloco;
  unsigned char hash_anterior[SHA256_DIGEST_LENGTH] = {0};
  MTRand rand = seedRand(1234567);

  for (numero_bloco = 1; numero_bloco <= TOTAL_BLOCOS; ++numero_bloco)
  {
    bloco.numero = numero_bloco;
    memcpy(bloco.hash_anterior, hash_anterior, SHA256_DIGEST_LENGTH);

    unsigned char hash[SHA256_DIGEST_LENGTH];

    if (numero_bloco == 1)
    {
      const char *mensagem = "The Times 03/Jan/2009 Chancellor on brink of "
                             "second bailout for banks";

      size_t tamanho_mensagem = strlen(mensagem);

      memcpy(bloco.data, mensagem, tamanho_mensagem);
      memset(bloco.data + tamanho_mensagem, 0,
             sizeof(bloco.data) - tamanho_mensagem);

      unsigned long int endereco_minerador = genRandLong(&rand) % NUM_ENDERECOS;
      bloco.data[183] = (unsigned char)endereco_minerador;

      minerar_bloco(&bloco, hash);
      inserir_enderecos_com_bitcoins(raiz, endereco_minerador);

      sistema->carteira[(uint32_t)bloco.data[DATA_LENGTH - 1]] +=
          RECOMPENSA_MINERACAO;
    }
    else
    {

      unsigned long int endereco_minerador = genRandLong(&rand) % NUM_ENDERECOS;

      gerar_transacoes_bloco(&bloco, endereco_minerador, sistema, rand, raiz);

      minerar_bloco(&bloco, hash);
      inserir_enderecos_com_bitcoins(raiz, endereco_minerador);

      sistema->carteira[(uint32_t)bloco.data[DATA_LENGTH - 1]] +=
          RECOMPENSA_MINERACAO;
    }

    inserir_bloco(blockchain, bloco, hash);

    FLAG_CHECA_CARTEIRAS = 0;
    while (FLAG_CHECA_CARTEIRAS != 1)
      FLAG_CHECA_CARTEIRAS = checa_zero_bitcoin(raiz, sistema);

    memset(bloco.data, 0, sizeof(bloco.data));
    memcpy(hash_anterior, hash, SHA256_DIGEST_LENGTH);
  }
}

int main()
{

  struct enderecos_bitcoin *raiz = NULL;
  struct bloco_minerado *blockchain = NULL;

  struct sistema_bitcoin sistema;

  iniciar_carteira(&sistema);
  processar_bloco(&raiz, &blockchain, &sistema);
  imprimir_blocos_minerados(blockchain, sistema);
  encontrar_maior_numero_bitcoins(sistema.carteira);
  free_blockchain(&blockchain);

  return EXIT_SUCCESS;
}
