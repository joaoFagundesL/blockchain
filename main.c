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

#define TOTAL_BLOCOS 16
#define MAX_TRANSACOES_BLOCO 61
#define NUM_ENDERECOS 256
#define DATA_LENGTH 184
#define RECOMPENSA_MINERACAO 50
#define MAX_BLOCOS_BUFFER 16

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

struct estatisticas {
  unsigned long int maior_transacao;
  unsigned long int menor_transacao;
  int array_quantidade_de_blocos_minerados[NUM_ENDERECOS];
};

struct sistema_bitcoin {
  uint32_t carteira[NUM_ENDERECOS];
};

struct enderecos_bitcoin {
  uint32_t chave;
  struct enderecos_bitcoin *prox;
};

void insere_arquivo(struct bloco_minerado **blockchain,
                    const char *nomeArquivo) {
  FILE *arquivo = fopen(nomeArquivo, "wb");
  if (arquivo == NULL) {
    fprintf(stderr, "Erro ao abrir o arquivo!");
    return;
  }

  if (blockchain == NULL || *blockchain == NULL) {
    fprintf(stderr, "Blockchain vazio!");
    fclose(arquivo);
    return;
  }

  struct bloco_minerado *bloco_atual = *blockchain;
  struct bloco_minerado buffer[MAX_BLOCOS_BUFFER];
  int contador = 0;

  while (bloco_atual != NULL) {
    if (contador < MAX_BLOCOS_BUFFER) {
      buffer[contador] = *bloco_atual;
      contador++;
    } else {
      fwrite(buffer, sizeof(struct bloco_minerado), contador, arquivo);
      contador = 0;
      continue;
    }

    bloco_atual = bloco_atual->prox;
  }

  if (contador > 0)
    fwrite(buffer, sizeof(struct bloco_minerado), contador, arquivo);

  fclose(arquivo);
}

struct bloco_minerado *le_arquivo(const char *nomeArquivo) {
  FILE *arquivo = fopen(nomeArquivo, "rb");
  if (arquivo == NULL) {
    fprintf(stderr, "Erro ao abrir o arquivo!");
    return NULL;
  }

  fseek(arquivo, 0, SEEK_END);
  long tamanho_arquivo = ftell(arquivo);
  rewind(arquivo);

  int num_blocos = tamanho_arquivo / sizeof(struct bloco_minerado);

  struct bloco_minerado *blockchain = (struct bloco_minerado *)malloc(
      num_blocos * sizeof(struct bloco_minerado));
  if (blockchain == NULL) {
    fprintf(stderr, "Erro ao alocar memória!");
    fclose(arquivo);
    return NULL;
  }

  fread(blockchain, sizeof(struct bloco_minerado), num_blocos, arquivo);

  fclose(arquivo);

  return blockchain;
}

void imprimir_bloco_arquivo_binario(const char *nome_arquivo) {
  FILE *arquivo = fopen(nome_arquivo, "rb");
  if (arquivo == NULL) {
    fprintf(stderr, "Erro ao abrir o arquivo!");
    return;
  }

  uint32_t numero_bloco;
  printf("Informe o numero do bloco que deseja procurar: ");
  scanf("%u", &numero_bloco);
  printf("\n");

  if (numero_bloco > TOTAL_BLOCOS || numero_bloco <= 0) {
    fprintf(stderr, "Bloco nao encontrado\n\n");
    return;
  }

  --numero_bloco;

  long deslocamento = numero_bloco * sizeof(struct bloco_minerado);

  fseek(arquivo, deslocamento, SEEK_SET);

  struct bloco_minerado bloco;
  fread(&bloco, sizeof(struct bloco_minerado), 1, arquivo);
  printf("Bloco %u:\n", bloco.bloco.numero);

  printf("Nonce: %u\n", bloco.bloco.nonce);

  printf("Hash do bloco: ");
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    printf("%02x", bloco.hash[i]);

  printf("\n");
  printf("Hash anterior: ");
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    printf("%02x", bloco.bloco.hash_anterior[i]);

  printf("\n");
  printf("Endereço do minerador: %u\n", bloco.bloco.data[DATA_LENGTH - 1]);

  if (bloco.bloco.numero != BLOCO_GENESIS) {
    for (int i = 0; i < MAX_TRANSACOES_BLOCO; i++) {
      uint32_t endereco_origem = bloco.bloco.data[i * 3];
      uint32_t endereco_destino = bloco.bloco.data[i * 3 + 1];
      uint32_t quantidade = bloco.bloco.data[i * 3 + 2];

      if (endereco_origem == 0 && endereco_destino == 0 && quantidade == 0)
        break;

      printf("Transação %d:\n", i + 1);

      printf("endereco de origem: %u\n", endereco_origem);
      printf("endereco de destino: %u\n", endereco_destino);
      printf("quantidade transferida: %u\n\n", quantidade);
    }
  }
  printf("\n\n");

  fclose(arquivo);
}

int checa_zero_bitcoin(struct enderecos_bitcoin **raiz,
                       struct sistema_bitcoin *sistema);

void iniciar_carteira(struct sistema_bitcoin *sistema) {
  memset(sistema->carteira, 0, sizeof(sistema->carteira));
}

void inserir_enderecos_com_bitcoins(struct enderecos_bitcoin **raiz,
                                    uint32_t chave) {
  struct enderecos_bitcoin *novo =
      (struct enderecos_bitcoin *)malloc(sizeof(struct enderecos_bitcoin));

  assert(novo != NULL);

  novo->chave = chave;
  novo->prox = *raiz;
  *raiz = novo;
}

int conta_enderecos(struct enderecos_bitcoin *raiz) {
  size_t cont = 0;
  if (!raiz)
    return 0;
  if (raiz)
    while (raiz) {
      raiz = raiz->prox;
      cont++;
    }
  return cont;
}

void verificar_hash_com_menos_transacao(struct bloco_minerado *blockchain,
                                        struct estatisticas *est) {
  if (blockchain == NULL)
    fprintf(stderr, "lista vazia\n");

  struct bloco_minerado *tmp = blockchain;

  printf("Menor numeros de transacoes = %lu nos hashs:\n",
         est->menor_transacao);

  while (tmp != NULL) {
    if (tmp->bloco.numero == BLOCO_GENESIS)
      tmp = tmp->prox;

    int count = 0;
    for (size_t i = 0; i < MAX_TRANSACOES_BLOCO; i++) {
      uint32_t endereco_origem = tmp->bloco.data[i * 3];
      uint32_t endereco_destino = tmp->bloco.data[i * 3 + 1];
      uint32_t quantidade = tmp->bloco.data[i * 3 + 2];

      if (endereco_origem == 0 && endereco_destino == 0 && quantidade == 0) {
        break;
      }

      count++;
    }

    if (count == est->menor_transacao || tmp->bloco.numero == BLOCO_GENESIS) {
      for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
        printf("%02x", tmp->hash[i]);
      printf("\n");
    }
    tmp = tmp->prox;
  }
  printf("\n");
}

void verificar_hash_com_mais_transacao(struct bloco_minerado *blockchain,
                                       struct estatisticas *est) {

  if (blockchain == NULL)
    fprintf(stderr, "lista vazia\n");

  struct bloco_minerado *tmp = blockchain;

  printf("Maior numeros de transacoes = %lu nos hashs:\n",
         est->maior_transacao);

  while (tmp != NULL) {
    int count = 0;
    for (size_t i = 0; i < MAX_TRANSACOES_BLOCO; i++) {
      uint32_t endereco_origem = tmp->bloco.data[i * 3];
      uint32_t endereco_destino = tmp->bloco.data[i * 3 + 1];
      uint32_t quantidade = tmp->bloco.data[i * 3 + 2];

      if (endereco_origem == 0 && endereco_destino == 0 && quantidade == 0) {
        break;
      }

      count++;
    }

    if (count == est->maior_transacao) {
      for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
        printf("%02x", tmp->hash[i]);
      printf("\n");
    }
    tmp = tmp->prox;
  }
  printf("\n");
}

void encontrar_maior_numero_bitcoins(uint32_t carteira[]) {
  int num_enderecos_max_bitcoins = 0;
  uint32_t max_bitcoins = 0;

  for (size_t i = 0; i < NUM_ENDERECOS; i++) {
    if (carteira[i] > max_bitcoins)
      max_bitcoins = carteira[i];
  }

  uint8_t *enderecos_max_bitcoins = malloc(sizeof(uint8_t));
  assert(enderecos_max_bitcoins != NULL);

  for (size_t i = 0; i < NUM_ENDERECOS; i++) {
    if (carteira[i] == max_bitcoins) {
      enderecos_max_bitcoins[num_enderecos_max_bitcoins] = i;
      num_enderecos_max_bitcoins++;

      enderecos_max_bitcoins =
          realloc(enderecos_max_bitcoins,
                  sizeof(uint8_t) * (num_enderecos_max_bitcoins + 1));

      assert(enderecos_max_bitcoins != NULL);
    }
  }

  printf("Maior numero de bitcoins = %u nos seguintes enderecos ",
         max_bitcoins);

  for (size_t i = 0; i < num_enderecos_max_bitcoins; i++)
    printf("%u ", enderecos_max_bitcoins[i]);
  printf("\n\n");

  free(enderecos_max_bitcoins);
  enderecos_max_bitcoins = NULL;
}

int escolhe_carteira(struct enderecos_bitcoin *raiz, uint32_t indice) {
  size_t i = 0;
  if (!raiz)
    return 0;
  else if (raiz) {
    while (i != indice && raiz != NULL) {
      raiz = raiz->prox;
      i++;
    }
    return raiz->chave;
  } else
    return 0;
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

    if (hash_result[0] == 0) {
      memcpy(hash, hash_result, SHA256_DIGEST_LENGTH);
      bloco->nonce = nonce;
      break;
    }

    nonce++;
  }
}

void atualizar_maior_menor_transacao(struct estatisticas *est,
                                     unsigned long int num_transacoes) {
  if (num_transacoes > est->maior_transacao)
    est->maior_transacao = num_transacoes;

  if (num_transacoes < est->menor_transacao)
    est->menor_transacao = num_transacoes;
}

void endereco_com_mais_blocos_minerados(struct estatisticas est) {
  int maior = 0;

  for (int i = 0; i < NUM_ENDERECOS; i++) {
    if (est.array_quantidade_de_blocos_minerados[i] > maior)
      maior = est.array_quantidade_de_blocos_minerados[i];
  }

  printf("Maior quantidade de blocos minerados = %d nos enderecos:\n", maior);
  for (int i = 0; i < NUM_ENDERECOS; i++) {
    if (est.array_quantidade_de_blocos_minerados[i] == maior)
      printf("%d ", i);
  }
  printf("\n\n");
}

void gerar_transacoes_bloco(struct bloco_nao_minerado *bloco,
                            struct sistema_bitcoin *sistema, MTRand *rand,
                            struct enderecos_bitcoin **raiz,
                            struct estatisticas *est, unsigned char *hash) {

  size_t num_transacoes = 0;
  uint32_t carteira_auxiliar[NUM_ENDERECOS] = {0};

  unsigned long int max_transacao =
      genRandLong(rand) % (MAX_TRANSACOES_BLOCO + 1);

  while (num_transacoes < max_transacao) {
    int valor_lista_origem = genRandLong(rand) % (conta_enderecos(*raiz));

    unsigned long int endereco_origem =
        escolhe_carteira(*raiz, valor_lista_origem);

    unsigned long int endereco_destino;
    unsigned long int quantidade;

    do {
      endereco_destino = genRandLong(rand) % NUM_ENDERECOS;
    } while (endereco_origem == endereco_destino);

    uint32_t saldo_disponivel = sistema->carteira[endereco_origem];

    quantidade = genRandLong(rand) % (saldo_disponivel + 1);

    sistema->carteira[endereco_origem] -= quantidade;
    carteira_auxiliar[endereco_destino] += quantidade;
    if (conta_enderecos(*raiz) > 1)
      checa_zero_bitcoin(raiz, sistema);

    bloco->data[num_transacoes * 3] = endereco_origem;
    bloco->data[num_transacoes * 3 + 1] = endereco_destino;
    bloco->data[num_transacoes * 3 + 2] = quantidade;

    num_transacoes++;
  }

  atualizar_maior_menor_transacao(est, max_transacao);

  unsigned long int endereco_minerador = genRandLong(rand) % NUM_ENDERECOS;

  ++est->array_quantidade_de_blocos_minerados[endereco_minerador];

  bloco->data[DATA_LENGTH - 1] = (unsigned char)endereco_minerador;

  for (size_t i = 0; i < NUM_ENDERECOS; i++) {
    if (carteira_auxiliar[i] > 0) {
      sistema->carteira[i] += carteira_auxiliar[i];
      inserir_enderecos_com_bitcoins(raiz, i);
    }
  }

  minerar_bloco(bloco, hash);
}

void inserir_bloco(struct bloco_minerado **blockchain,
                   struct bloco_nao_minerado *bloco, unsigned char *hash) {

  struct bloco_minerado *novo_bloco =
      (struct bloco_minerado *)malloc(sizeof(struct bloco_minerado));

  assert(novo_bloco != NULL);

  memcpy(&(novo_bloco->bloco), bloco, sizeof(struct bloco_nao_minerado));
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

void imprimir_blocos_minerados(struct bloco_minerado *blockchain) {
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
    printf("Endereço do minerador: %u\n", atual->bloco.data[DATA_LENGTH - 1]);

    if (atual->bloco.numero != BLOCO_GENESIS) {
      for (int i = 0; i < MAX_TRANSACOES_BLOCO; i++) {
        uint32_t endereco_origem = atual->bloco.data[i * 3];
        uint32_t endereco_destino = atual->bloco.data[i * 3 + 1];
        uint32_t quantidade = atual->bloco.data[i * 3 + 2];

        if (endereco_origem == 0 && endereco_destino == 0 && quantidade == 0)
          break;

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

void remove_no(struct enderecos_bitcoin **raiz, uint32_t chave) {
  if (*raiz == NULL)
    return;

  if ((*raiz)->chave == chave) {
    struct enderecos_bitcoin *temp = *raiz;

    *raiz = (*raiz)->prox;

    free(temp);

    temp = NULL;

    return;
  }

  struct enderecos_bitcoin *atual = *raiz;
  struct enderecos_bitcoin *anterior = NULL;

  while (atual != NULL && atual->chave != chave) {
    anterior = atual;
    atual = atual->prox;
  }

  if (atual == NULL)
    return;
  anterior->prox = atual->prox;
  free(atual);
}

int checa_zero_bitcoin(struct enderecos_bitcoin **raiz,
                       struct sistema_bitcoin *sistema) {

  if (*raiz == NULL)
    return 1;
  struct enderecos_bitcoin *atual = *raiz;

  while (atual) {

    if (sistema->carteira[atual->chave] <= 0) {
      remove_no(raiz, atual->chave);
      return 0;
    }
    atual = atual->prox;
  }
  return 1;
}

void imprime_lista(struct enderecos_bitcoin *raiz) {
  while (raiz != NULL) {
    /* Supondo que o campo a ser impresso seja 'valor' */
    printf("%d ", raiz->chave);
    raiz = raiz->prox;
  }

  printf("\n");
}

void free_blockchain(struct bloco_minerado **blockchain) {
  while (*blockchain != NULL) {
    struct bloco_minerado *proximo = (*blockchain)->prox;
    free(*blockchain);
    *blockchain = proximo;
  }
}

void processar_bloco(struct enderecos_bitcoin **raiz,
                     struct bloco_minerado **blockchain,
                     struct sistema_bitcoin *sistema,
                     struct estatisticas *est) {

  struct bloco_nao_minerado bloco;

  uint8_t FLAG_CHECA_CARTEIRAS;

  size_t numero_bloco;
  unsigned char hash_anterior[SHA256_DIGEST_LENGTH] = {0};
  MTRand rand = seedRand(1234567);

  for (numero_bloco = 1; numero_bloco <= TOTAL_BLOCOS; ++numero_bloco) {
    bloco.numero = numero_bloco;
    memcpy(bloco.hash_anterior, hash_anterior, SHA256_DIGEST_LENGTH);

    unsigned char hash[SHA256_DIGEST_LENGTH];

    if (numero_bloco == 1) {
      const char *mensagem = "The Times 03/Jan/2009 Chancellor on brink of "
                             "second bailout for banks";

      size_t tamanho_mensagem = strlen(mensagem);

      memcpy(bloco.data, mensagem, tamanho_mensagem);
      memset(bloco.data + tamanho_mensagem, 0, DATA_LENGTH - tamanho_mensagem);

      unsigned long int endereco_minerador = genRandLong(&rand) % NUM_ENDERECOS;
      bloco.data[183] = (unsigned char)endereco_minerador;

      minerar_bloco(&bloco, hash);

      ++est->array_quantidade_de_blocos_minerados[endereco_minerador];

      sistema->carteira[(uint32_t)bloco.data[DATA_LENGTH - 1]] +=
          RECOMPENSA_MINERACAO;

      inserir_enderecos_com_bitcoins(raiz, endereco_minerador);
    } else {

      /* Depois de gerar as transacoes do bloco a funcao vai chamar a funcao de
       * minerar com os parametros corretos */
      gerar_transacoes_bloco(&bloco, sistema, &rand, raiz, est, hash);

      sistema->carteira[(uint32_t)bloco.data[DATA_LENGTH - 1]] +=
          RECOMPENSA_MINERACAO;

      if (sistema->carteira[(uint32_t)bloco.data[DATA_LENGTH - 1]] == 50)
        inserir_enderecos_com_bitcoins(raiz,
                                       (uint32_t)bloco.data[DATA_LENGTH - 1]);
    }

    inserir_bloco(blockchain, &bloco, hash);

    FLAG_CHECA_CARTEIRAS = 0;
    while (FLAG_CHECA_CARTEIRAS != 1 && conta_enderecos(*raiz) > 1) {
      FLAG_CHECA_CARTEIRAS = checa_zero_bitcoin(raiz, sistema);
    }

    memset(bloco.data, 0, sizeof(bloco.data));
    memcpy(hash_anterior, hash, SHA256_DIGEST_LENGTH);
  }
}

int main() {

  struct enderecos_bitcoin *raiz = NULL;
  struct bloco_minerado *blockchain = NULL;

  struct estatisticas est = {.maior_transacao = 0,
                             .menor_transacao = MAX_TRANSACOES_BLOCO + 1};

  memset(est.array_quantidade_de_blocos_minerados, 0,
         sizeof(est.array_quantidade_de_blocos_minerados));

  struct sistema_bitcoin sistema;

  iniciar_carteira(&sistema);
  processar_bloco(&raiz, &blockchain, &sistema, &est);

  insere_arquivo(&blockchain, "blocos.bin");

  struct bloco_minerado *test = le_arquivo("blocos.bin");
  // imprimir_blocos_minerados(blockchain);

  char opcao;
  while (1) {
    printf("[a] => Endereco com mais bitcoins e o numero de bitcoins dele\n[b] "
           "=> Endereco que minerou mais blocos\n[c] "
           "=> Hash do bloco com mais transacoes e o numero de transacoes "
           "\n[d] => Hash com menos transacoes e o numero de transacoes\n[e] "
           "=> Quantidade media de bitcoins por bloco\n[f] => Imprimir todos "
           "os campos de um dado "
           "bloco\n============================================================"
           "==\n--> ");

    scanf(" %c", &opcao);
    if (opcao == 'a')
      encontrar_maior_numero_bitcoins(sistema.carteira);
    else if (opcao == 'b')
      endereco_com_mais_blocos_minerados(est);
    else if (opcao == 'c')
      verificar_hash_com_mais_transacao(blockchain, &est);
    else if (opcao == 'd')
      verificar_hash_com_menos_transacao(blockchain, &est);
    else if (opcao == 'e')
      printf("todo");
    else if (opcao == 'f')
      imprimir_bloco_arquivo_binario("blocos.bin");
    else
      break;
  }

  free_blockchain(&blockchain);

  return EXIT_SUCCESS;
}
