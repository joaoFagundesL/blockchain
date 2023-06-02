#include "blockchain.h"
#include <stdint.h>

void insere_arquivo(struct bloco_minerado **blockchain,
                    const char *nomeArquivo) {

  FILE *arquivo = fopen(nomeArquivo, "rb");

  handle_file(&arquivo, nomeArquivo, "wb");

  /* Verificando se tem alguem na lista para mandar para o arquivo */
  if (blockchain == NULL || *blockchain == NULL) {
    fprintf(stderr, "Blockchain vazio!");
    fclose(arquivo);
    return;
  }

  struct bloco_minerado *bloco_atual = *blockchain;
  struct bloco_minerado buffer[MAX_BLOCOS_BUFFER];
  int contador = 0;

  while (bloco_atual != NULL) {
    /* atualizo o meu array com um determinado bloco enquanto nao tiver 16
     * blocos */
    if (contador < MAX_BLOCOS_BUFFER) {
      buffer[contador] = *bloco_atual;
      contador++;
    } else {

      /* Mando para o arquivo 16 blocos */
      fwrite(buffer, sizeof(struct bloco_minerado), contador, arquivo);

      /* Comeca a contagem até 16 novamente */
      contador = 0;
      continue;
    }

    bloco_atual = bloco_atual->prox;
  }

  /* Vamos supor que possui 17 blocos, sendo assim ficaria faltando 1 bloco
   * para mandar para o arquivo, já que reuno apenas 16. Aqui eu envio os que
   * sobram no final */
  if (contador > 0)
    fwrite(buffer, sizeof(struct bloco_minerado), contador, arquivo);

  fclose(arquivo);
}

/* Aqui eu garanto que eu criei meu arquivo binario com os blocos de forma
 * correta. Retorno uma struct para fins de comparacao com a blockchain
 * original. Caso sejam iguais meu arquivo foi criado corretamente */
struct bloco_minerado *le_arquivo(const char *nomeArquivo) {

  FILE *arquivo = fopen(nomeArquivo, "rb");
  if (arquivo == NULL) {
    fprintf(stderr, "Erro ao abrir o arquivo!");
    return NULL;
  }

  /* Indo para o final do arquivo para calcular a quantidade de blocos */
  fseek(arquivo, 0, SEEK_END);
  long tamanho_arquivo = ftell(arquivo);

  /* Voltando para o inicio (poderia usar fseek(arquivo, 0, SEEK_SET)) */
  rewind(arquivo);

  int num_blocos = tamanho_arquivo / sizeof(struct bloco_minerado);

  /* Tamanho total da blockain baseada na contagem de blocos do arquivo, saber
   * isso agiliza o fread, pois ja mando o numero de blocos que quero ler*/
  struct bloco_minerado *blockchain = (struct bloco_minerado *)malloc(
      num_blocos * sizeof(struct bloco_minerado));

  if (blockchain == NULL) {
    fprintf(stderr, "Erro ao alocar memória!");
    fclose(arquivo);
    return NULL;
  }

  /* Economizando tempo de leitura pois eu leio de uma vez só e não um bloco
   * de cada */
  fread(blockchain, sizeof(struct bloco_minerado), num_blocos, arquivo);

  fclose(arquivo);

  return blockchain;
}

void criar_arquivo_indice(const char *nome_arquivo,
                          const char *nome_arquivo_indice) {

  /* Ler os dados do arquivo que possui os blocos */
  FILE *arquivo = fopen(nome_arquivo, "rb");
  if (arquivo == NULL) {
    fprintf(stderr, "Erro ao abrir o arquivo de dados!");
    return;
  }

  /* Criando o arquivo de indice dos mineradores */
  FILE *indice = fopen(nome_arquivo_indice, "rb");

  handle_file(&indice, nome_arquivo_indice, "wb");

  struct bloco_minerado bloco;
  struct indice_registro registros[MAX_ENDERECO_BUFFER];
  int contador = 0;

  /* A leitura eu faço com base no arquivo que ja tenho sequencialmente
   * construido */
  while (fread(&bloco, sizeof(struct bloco_minerado), 1, arquivo) > 0) {
    registros[contador].endereco = bloco.bloco.data[DATA_LENGTH - 1];

    /* [a] [b] [c] [d] [bloco que foi lido]
     * o ftell me retornaria na posicao logo apos o bloco que foi lido, logo
     * se eu subtrair o tamanho desse bloco eu tenho o offset dele */

    /* Ftell retorna aonde estou no arquivo. Se eu faço ftell - sizeof eu
     * volto para o bloco que acabou de ser lido, ou seja consigo o offset
     * dele */
    registros[contador].offset = ftell(arquivo) - sizeof(struct bloco_minerado);

    contador++;

    /* Depois de reunir os 256 enderecos eu faço um fwrite e reseto a contagem
     * para 0 */
    if (contador == MAX_ENDERECO_BUFFER) {
      fwrite(registros, sizeof(struct indice_registro), contador, indice);
      contador = 0;
    }
  }

  /* Provavelmente nunca vai entrar aqui mas por questão de seguranca caso
   * falte algum minerador eu escrevo ele */
  if (contador > 0)
    fwrite(registros, sizeof(struct indice_registro), contador, indice);

  fclose(arquivo);
  fclose(indice);
}

/* Vou utilizar a mesma logica para construir o arquivo de indice do nonce */
void criar_arquivo_indice_nonce(const char *nome_arquivo,
                                const char *nome_arquivo_indice) {

  /* Ler os dados do arquivo que possui os blocos */
  FILE *arquivo = fopen(nome_arquivo, "rb");
  if (arquivo == NULL) {
    fprintf(stderr, "Erro ao abrir o arquivo de dados!");
    return;
  }

  /* Criando o arquivo de indice dos mineradores */
  FILE *indice = fopen(nome_arquivo_indice, "rb");
  handle_file(&indice, nome_arquivo_indice, "wb");

  struct bloco_minerado bloco;
  struct indice_nonce registros[MAX_NONCE_BUFFER];
  int contador = 0;

  /* A leitura eu faço com base no arquivo que ja tenho sequencialmente
   * construido */
  while (fread(&bloco, sizeof(struct bloco_minerado), 1, arquivo) > 0) {
    registros[contador].nonce = bloco.bloco.nonce;

    /* Ftell retorna aonde estou no arquivo. Se eu faço ftell - sizeof eu
     * volto para o bloco que acabou de ser lido, ou seja consigo o offset
     * dele */
    registros[contador].offset = ftell(arquivo) - sizeof(struct bloco_minerado);

    contador++;

    if (contador == MAX_NONCE_BUFFER) {
      fwrite(registros, sizeof(struct indice_nonce), contador, indice);
      contador = 0;
    }
  }

  if (contador > 0)
    fwrite(registros, sizeof(struct indice_nonce), contador, indice);

  fclose(arquivo);
  fclose(indice);
}

/* Funcao para mostrar os dados de um código para evitar ficar repetindo toda
 * hora */
void exibir_dados_bloco(struct bloco_minerado bloco) {
  printf("\n==================================================================="
         "===\n");
  printf("Bloco %d\n", bloco.bloco.numero);
  printf("Nonce = %u\n", bloco.bloco.nonce);

  printf("Hash: ");
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    printf("%02x", bloco.hash[i]);
  printf("\n");

  printf("Hash anterior: ");
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    printf("%02x", bloco.bloco.hash_anterior[i]);
  printf("\n");

  printf("Endereco do minerador = %d\n", bloco.bloco.data[DATA_LENGTH - 1]);

  /* Só imprime transacao se nao for o genesis */
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
}

void imprimir_blocos_nonce(const char *nome_arquivo,
                           const char *nome_arquivo_indice) {

  /* Arquivo com os blocos armazenados sequencialmente */
  FILE *arquivo = fopen(nome_arquivo, "rb");
  if (arquivo == NULL) {
    fprintf(stderr, "Erro ao abrir o arquivo de dados!");
    return;
  }

  /* Arquivo de indice que foi criado */
  FILE *indice = fopen(nome_arquivo_indice, "rb");
  if (indice == NULL) {
    fclose(arquivo);
    fprintf(stderr, "Erro ao abrir o arquivo de índice!");
    return;
  }

  /* Leitura de dados */
  uint32_t nonce;
  printf("Informe o nonce desejado: ");
  scanf("%u", &nonce);

  struct indice_nonce registro;
  struct bloco_minerado bloco;

  /* Controle para imprimir os dados de acordo com o n fornecido pelo
   * usuario*/
  int blocos_impressos = 0;

  while (fread(&registro, sizeof(struct indice_nonce), 1, indice) > 0) {

    if (registro.nonce == nonce) {

      /* Uso o offset e ja vou direto para o bloco desejado no meu arquivo */
      fseek(arquivo, registro.offset, SEEK_SET);

      /* Leio o bloco que estava naquele offset */
      fread(&bloco, sizeof(struct bloco_minerado), 1, arquivo);

      /* Imprimir todos os dados daquele bloco */
      exibir_dados_bloco(bloco);

      blocos_impressos++;
    }
  }

  if (blocos_impressos == 0)
    fprintf(stderr, "Nenhum bloco com o nonce informado!\n\n");

  fclose(arquivo);
  fclose(indice);
}

void imprimir_blocos_endereco(const char *nome_arquivo,
                              const char *nome_arquivo_indice) {

  /* Arquivo com os blocos armazenados sequencialmente */
  FILE *arquivo = fopen(nome_arquivo, "rb");
  if (arquivo == NULL) {
    fprintf(stderr, "Erro ao abrir o arquivo de dados!");
    return;
  }

  /* Arquivo de indice que foi criado */
  FILE *indice = fopen(nome_arquivo_indice, "rb");
  if (indice == NULL) {
    fclose(arquivo);
    fprintf(stderr, "Erro ao abrir o arquivo de índice!");
    return;
  }

  /* Leitura de dados */
  int endereco;
  printf("Informe o endereco desejado: ");
  scanf("%d", &endereco);

  int n;
  printf("Informe a quantidade de blocos: ");
  scanf("%d", &n);

  struct indice_registro registro;
  struct bloco_minerado bloco;

  /* Controle para imprimir os dados de acordo com o n fornecido pelo
   * usuario*/
  int blocos_impressos = 0;

  while (blocos_impressos < n &&
         fread(&registro, sizeof(struct indice_registro), 1, indice) > 0) {

    if (registro.endereco == endereco) {

      /* Uso o offset e ja vou direto para o bloco desejado no meu arquivo */
      fseek(arquivo, registro.offset, SEEK_SET);

      /* Leio o bloco que estava naquele offset */
      fread(&bloco, sizeof(struct bloco_minerado), 1, arquivo);

      /* Imprimir todos os dados daquele bloco */
      exibir_dados_bloco(bloco);
      if (blocos_impressos >= n)
        break;

      blocos_impressos++;
    }
  }

  if (blocos_impressos == 0)
    fprintf(stderr, "Nenhum bloco encontrado!\n\n");

  fclose(arquivo);
  fclose(indice);
}

/* Apenas para verificar se o arquivo de indice dos mineradores foi criado
 * corretamente */
void imprimir_arquivo_indice(const char *nomeArquivoIndice) {
  FILE *indice = fopen(nomeArquivoIndice, "rb");
  if (indice == NULL) {
    fprintf(stderr, "Erro ao abrir o arquivo de índice!");
    return;
  }

  struct indice_registro registro;

  while (fread(&registro, sizeof(struct indice_registro), 1, indice) > 0) {
    printf("Endereco: %d, Offset: %ld\n", registro.endereco, registro.offset);
  }

  fclose(indice);
}

/* Dado um bloco n pelo usuario informar todos os dados daquele bloco */
void imprimir_bloco_arquivo_binario(const char *nome_arquivo) {
  FILE *arquivo = fopen(nome_arquivo, "rb");
  if (arquivo == NULL) {
    fprintf(stderr, "Erro ao abrir o arquivo!");
    return;
  }

  uint32_t numero_bloco;
  printf("Informe o numero do bloco que deseja procurar: ");
  scanf("%u", &numero_bloco);

  if (numero_bloco > TOTAL_BLOCOS || numero_bloco <= 0) {
    fprintf(stderr, "Bloco nao encontrado\n\n");
    return;
  }

  /* Isso garante que eu imprima o numero do bloco de forma correta, pois
   * estava imprimindo sempre um bloco a frente daquele desejado */
  --numero_bloco;

  /* Calculo do offset necessario que garante uma leitura eficiente pois eu ja
   * vou diretamente naquele bloco sem precisar ler um por um do arquivo */
  long deslocamento = numero_bloco * sizeof(struct bloco_minerado);

  /* Posiciono no bloco desejado */
  fseek(arquivo, deslocamento, SEEK_SET);

  /* Agora basta ler e imprimir as informacoes */
  struct bloco_minerado bloco;
  fread(&bloco, sizeof(struct bloco_minerado), 1, arquivo);

  exibir_dados_bloco(bloco);

  fclose(arquivo);
}

/* Preencher todas as posicoes do vetor com 0 */
void iniciar_carteira(struct sistema_bitcoin *sistema) {
  memset(sistema->carteira, 0, sizeof(sistema->carteira));
}

/* Insere na lista enderecos que possuem bitcoins */
void inserir_enderecos_com_bitcoins(struct enderecos_bitcoin **raiz,
                                    struct estatisticas **est, uint32_t chave) {
  struct enderecos_bitcoin *novo =
      (struct enderecos_bitcoin *)malloc(sizeof(struct enderecos_bitcoin));

  assert(novo != NULL);

  novo->chave = chave;
  novo->prox = *raiz;
  *raiz = novo;
  (*est)->tamanho_lista_enderecos_com_bitcoin++;
}

/* Sempre recebo a minha struct est com os campos de menor e maior transacao
 * atualizados, basta eu saber de quem é o hash */
void verificar_hash_com_menos_transacao(struct bloco_minerado *blockchain,
                                        struct estatisticas *est) {

  if (blockchain == NULL) {
    fprintf(stderr, "lista vazia\n");
    return;
  }

  struct bloco_minerado *tmp = blockchain;
  printf("Menor numero de transacoes = %lu nos hashs:\n", est->menor_transacao);

  while (tmp != NULL) {
    if (tmp->bloco.numero == BLOCO_GENESIS)
      /* Como o genesis nao tem transacao eu posso ir para o proximo elemento
       * da lista */
      tmp = tmp->prox;

    int count = 0;

    for (size_t i = 0; i < MAX_TRANSACOES_BLOCO; i++) {
      uint32_t endereco_origem = tmp->bloco.data[i * 3];
      uint32_t endereco_destino = tmp->bloco.data[i * 3 + 1];
      uint32_t quantidade = tmp->bloco.data[i * 3 + 2];

      /* Aqui eu sei quando acaba a transacao do bloco */
      if (endereco_origem == 0 && endereco_destino == 0 && quantidade == 0) {
        break;
      }

      count++;
    }

    /* Printo o hash caso encontre */
    if ((unsigned long int)count == est->menor_transacao ||
        tmp->bloco.numero == BLOCO_GENESIS) {
      for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
        printf("%02x", tmp->hash[i]);
    }

    /* Avanca ate ser nulo */
    tmp = tmp->prox;
  }
  printf("\n\n");
}

/* Mesma logica da funcao anterior */
void verificar_hash_com_mais_transacao(struct bloco_minerado *blockchain,
                                       struct estatisticas *est) {

  if (blockchain == NULL) {
    fprintf(stderr, "lista vazia\n");
    return;
  }

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

    if ((unsigned long int)count == est->maior_transacao) {
      for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
        printf("%02x", tmp->hash[i]);
      printf("\n");
    }
    tmp = tmp->prox;
  }
  printf("\n\n");
}

void encontrar_maior_numero_bitcoins(uint32_t carteira[]) {

  /* Variável de controle pois preciso imprimir todos os enderecos em caso de
   * empate */
  int num_enderecos_max_bitcoins = 0;

  /* Inicia com 0 para garantir que sempre vai ter alguém maior que 0 */
  uint32_t max_bitcoins = 0;

  /* Acha o maior número de bitcoins */
  for (size_t i = 0; i < NUM_ENDERECOS; i++) {
    if (carteira[i] > max_bitcoins)
      max_bitcoins = carteira[i];
  }

  /* Como eu não sei quantos enderecos possuem, faço um vetor dinâmico */
  uint8_t *enderecos_max_bitcoins = malloc(sizeof(uint8_t));
  assert(enderecos_max_bitcoins != NULL);

  for (size_t i = 0; i < NUM_ENDERECOS; i++) {
    if (carteira[i] == max_bitcoins) {
      enderecos_max_bitcoins[num_enderecos_max_bitcoins] = i;
      num_enderecos_max_bitcoins++;

      /* Sempre que tiver um endereco, eu aumento meu vetor */
      enderecos_max_bitcoins =
          realloc(enderecos_max_bitcoins,
                  sizeof(uint8_t) * (num_enderecos_max_bitcoins + 1));

      assert(enderecos_max_bitcoins != NULL);
    }
  }

  printf("Maior numero de bitcoins = %u nos seguintes enderecos ",
         max_bitcoins);

  for (int i = 0; i < num_enderecos_max_bitcoins; i++)
    printf("%u ", enderecos_max_bitcoins[i]);
  printf("(Com %d blocos minerados)\n", TOTAL_BLOCOS);

  printf("\n\n");

  /* Libero vetor dinamico */
  free(enderecos_max_bitcoins);
  enderecos_max_bitcoins = NULL;
}

int escolhe_carteira(struct enderecos_bitcoin *raiz, uint32_t indice) {
  if (!raiz)
    return 0;

  for (uint32_t i = 0; i < indice && raiz != NULL; ++i)
    raiz = raiz->prox;

  return raiz->chave;
}

void minerar_bloco(struct bloco_nao_minerado *bloco, unsigned char *hash,
                   struct estatisticas **est) {
  uint32_t nonce = 0;

  unsigned char hash_result[SHA256_DIGEST_LENGTH];

  do {
    bloco->nonce = nonce;
    SHA256((unsigned char *)bloco, sizeof(struct bloco_nao_minerado),
           hash_result);
    nonce++;
  } while (hash_result[0] != 0);

  /* Atualizo o hash que veio como parametro para atualiza-lo na chamada
   * funcao
   */
  memcpy(hash, hash_result, SHA256_DIGEST_LENGTH);

  if ((*est)->quantidade_minerados == 1000) {
    printf("%lu Blocos minerados...\n", (*est)->quantidade_minerados);
    (*est)->quantidade_minerados = 0;
  } else
    (*est)->quantidade_minerados++;
}

/* Funcao que garante os campos sempre atualizados da struct estatisticas. */
void atualizar_maior_menor_transacao(struct estatisticas *est,
                                     unsigned long int num_transacoes) {
  if (num_transacoes > est->maior_transacao)
    est->maior_transacao = num_transacoes;

  if (num_transacoes < est->menor_transacao)
    est->menor_transacao = num_transacoes;
}

void endereco_com_mais_blocos_minerados(struct estatisticas est) {

  int maior = 0;

  /* Encontra o maior dentro do array */
  for (int i = 0; i < NUM_ENDERECOS; i++) {
    if (est.array_quantidade_de_blocos_minerados[i] > maior)
      maior = est.array_quantidade_de_blocos_minerados[i];
  }

  /* Agora eu acho o endereco de onde o maior estiver.
   * Cada posicao do array é um minerador e cada valor é a quantidade de
   * blocos minerados
   * */
  printf("Maior quantidade de blocos minerados = %d nos enderecos:\n", maior);

  for (int i = 0; i < NUM_ENDERECOS; i++) {
    if (est.array_quantidade_de_blocos_minerados[i] == maior)
      printf("%d ", i);
  }
  printf("\n\n");
}

void quantidade_media_bitcoins_bloco(struct estatisticas est) {
  printf("Quantidade media de bitcoins por bloco = %.2f\n\n",
         est.quantidade_total_bitcoins_por_bloco / (TOTAL_BLOCOS - 1));
}

/* Funcao que remove elemento da lista */
void remove_no(struct enderecos_bitcoin **raiz, struct estatisticas **est,
               uint32_t chave) {
  if (*raiz == NULL)
    return;

  /* Remove no inicio da lista */
  if ((*raiz)->chave == chave) {
    struct enderecos_bitcoin *temp = *raiz;
    *raiz = (*raiz)->prox;

    free(temp);
    temp = NULL;

    (*est)->tamanho_lista_enderecos_com_bitcoin--;
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
  (*est)->tamanho_lista_enderecos_com_bitcoin--;
}

/* Se alguem da lista tiver 0 bitcoins eu removo ele */
int checa_zero_bitcoin(struct enderecos_bitcoin **raiz,
                       struct estatisticas **est,
                       struct sistema_bitcoin *sistema) {

  if (*raiz == NULL)
    return 1;

  struct enderecos_bitcoin *atual = *raiz;

  while (atual) {
    if (sistema->carteira[atual->chave] <= 0) {
      remove_no(raiz, est, atual->chave);
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

/* O rand deve ser passado por referencia para que seja alterado na funcao
 * onde foi definida a seed */
void gerar_transacoes_bloco(struct bloco_nao_minerado *bloco,
                            struct sistema_bitcoin *sistema, MTRand *rand,
                            struct enderecos_bitcoin **raiz,
                            struct estatisticas **est, unsigned char *hash) {

  unsigned long int endereco_destino;
  unsigned long int quantidade;

  /* Carteira auxiliar que facilita da hora de manipular */
  uint32_t carteira_auxiliar[NUM_ENDERECOS] = {0};

  /* +1 pois garante que incluo o valor MAX_TRANSACOES_BLOCO */
  unsigned long int max_transacao =
      genRandLong(rand) % (MAX_TRANSACOES_BLOCO + 1);

  for (unsigned long int i = 0; i < max_transacao; ++i) {

    /* O sorteio é feito baseado na quantidade de elementos da lista */
    int valor_lista_origem =
        genRandLong(rand) % (*est)->tamanho_lista_enderecos_com_bitcoin;

    /* essa função escolhe a carteira de origem, dentre as opções que estão
     * contidas na lista que tão as carteiras que tem bitcoin*/
    unsigned long int endereco_origem =
        escolhe_carteira(*raiz, valor_lista_origem);

    /* O endereco de destino e origem nunca vao ser iguais por conta desse do
     * while (opcional) */
    do {
      endereco_destino = genRandLong(rand) % NUM_ENDERECOS;
    } while (endereco_origem == endereco_destino);

    // if (sistema->carteira[endereco_origem] == 0)
    // quantidade = 0;
    // else
    quantidade = genRandLong(rand) % (sistema->carteira[endereco_origem] + 1);

    /* Sempre armazenando a quantidade de bitcoins assim para manter controle
     * da media */
    (*est)->quantidade_total_bitcoins_por_bloco += quantidade;

    /* Retiro a quantidade de bitcoins do endereco de origem  */
    sistema->carteira[endereco_origem] -= quantidade;

    /* Aqui so atualizo a carteira_auxiliar, a carteira do sistema eu atualizo
     * depois de fazer a mineracao */
    carteira_auxiliar[endereco_destino] += quantidade;

    /* Mantem controle para evitar que nao haja alguem transferindo bitcoins
     * sem possuir*/
    if ((*est)->tamanho_lista_enderecos_com_bitcoin > 1)
      checa_zero_bitcoin(raiz, est, sistema);

    bloco->data[i * 3] = endereco_origem;
    bloco->data[i * 3 + 1] = endereco_destino;
    bloco->data[i * 3 + 2] = quantidade;
  }

  atualizar_maior_menor_transacao(*est, max_transacao);

  /* Ultima coisa a ser feita é gerar o minerador */
  unsigned long int endereco_minerador = genRandLong(rand) % NUM_ENDERECOS;

  /* Incremento a quantidade de blocos minerados com base no valor do
   * minerador sorteado */
  (*est)->array_quantidade_de_blocos_minerados[endereco_minerador] += 1;

  /* data[183] é o endereco do minerador */
  bloco->data[DATA_LENGTH - 1] = (unsigned char)endereco_minerador;

  minerar_bloco(bloco, hash, est);

  /* Atualizando a carteira do sistema apos a mineracao */
  for (int i = 0; i < NUM_ENDERECOS; i++) {
    if (carteira_auxiliar[i] > 0) {
      sistema->carteira[i] += carteira_auxiliar[i];
      inserir_enderecos_com_bitcoins(raiz, est, i);
    }
  }
}

void inserir_bloco(struct bloco_minerado **blockchain,
                   struct bloco_nao_minerado *bloco, unsigned char *hash) {

  struct bloco_minerado *novo_bloco =
      (struct bloco_minerado *)malloc(sizeof(struct bloco_minerado));

  assert(novo_bloco != NULL);

  /* Passo o endereco do novo bloco para copiar  */
  memcpy(&(novo_bloco->bloco), bloco, sizeof(struct bloco_nao_minerado));

  /* Copia o hash */;
  memcpy(novo_bloco->hash, hash, SHA256_DIGEST_LENGTH);

  novo_bloco->prox = NULL;

  /* Insercao */
  if (*blockchain == NULL)
    *blockchain = novo_bloco;

  else {
    struct bloco_minerado *atual = *blockchain;

    while (atual->prox != NULL)
      atual = atual->prox;

    atual->prox = novo_bloco;
  }
}

void handle_file(FILE **file, const char *filename, const char *write_mode) {

  if (*file != NULL) {
    char opcao;

    printf("Arquivo '%s' encontrado (pode estar desatualizado caso alguma "
           "alteracao tenha sido feita no programa). Deseja apagar e criar um "
           "novo? (y/n): ",
           filename);

    scanf(" %c", &opcao);
    printf("\n");

    if (opcao == 'y') {
      fclose(*file);

      *file = fopen(filename, write_mode);

      if (file == NULL) {
        fprintf(stderr, "Erro ao criar o arquivo!");
        return;
      }
    }

  } else {
    printf("Arquivo '%s' nao encontrado, criando um novo...\n\n", filename);

    *file = fopen(filename, write_mode);

    if (*file == NULL) {
      fprintf(stderr, "Erro ao criar o arquivo!");
      return;
    }
  }
}

/* Funcao que imprime todos os blocos */
void imprimir_blocos_minerados(struct bloco_minerado *blockchain,
                               const char *filename) {

  struct bloco_minerado *atual = blockchain;

  FILE *file = fopen(filename, "r");
  handle_file(&file, filename, "w");

  while (atual != NULL) {
    fprintf(file, "Bloco %u:\n", atual->bloco.numero);
    fprintf(file, "Nonce: %u\n", atual->bloco.nonce);
    fprintf(file, "Hash do bloco: ");
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
      fprintf(file, "%02x", atual->hash[i]);

    fprintf(file, "\n");
    fprintf(file, "Hash anterior: ");
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
      fprintf(file, "%02x", atual->bloco.hash_anterior[i]);

    fprintf(file, "\n");
    fprintf(file, "Endereço do minerador: %u\n",
            atual->bloco.data[DATA_LENGTH - 1]);

    if (atual->bloco.numero != BLOCO_GENESIS) {
      for (int i = 0; i < MAX_TRANSACOES_BLOCO; i++) {
        uint32_t endereco_origem = atual->bloco.data[i * 3];
        uint32_t endereco_destino = atual->bloco.data[i * 3 + 1];
        uint32_t quantidade = atual->bloco.data[i * 3 + 2];

        /* Na ha mais transacoes */
        if (endereco_origem == 0 && endereco_destino == 0 && quantidade == 0)
          break;

        fprintf(file, "Transação %d:\n", i + 1);

        fprintf(file, "endereco de origem: %u\n", endereco_origem);
        fprintf(file, "endereco de destino: %u\n", endereco_destino);
        fprintf(file, "quantidade transferida: %u\n\n", quantidade);
      }
    }
    fprintf(file, "\n");

    atual = atual->prox;
  }
}

void ordernar_vetor(struct transacao_ordenada *arr, int n) {
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      if (arr[i].quantidade > arr[j].quantidade) {
        struct transacao_ordenada tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
      }
    }
  }
}

void transacoes_crescente(struct bloco_minerado *blockchain) {
  if (blockchain == NULL)
    return;

  int n;
  printf("Informe a quantidade de blocos: ");
  scanf("%d", &n);

  /* - 1 porque o genesis vai contar ja que ele nao tem transacao */
  if (n < 0) {
    printf("Valor nao pode ser negativo\n\n");
    return;
  }

  if (n > TOTAL_BLOCOS - 1) {
    printf("Valor invalido\n\n");
    return;
  }

  /* Uma array de struct que vai armazenar duas informacoes: bloco e a
   * quantidade de transacoes */
  struct transacao_ordenada arr[n];
  struct bloco_minerado *curr = blockchain;

  /* Se for o genesis eu vou para o proximo */
  if (curr->bloco.numero == BLOCO_GENESIS)
    curr = curr->prox;

  for (int i = 0; i < n; i++) {
    int quantidade_transacao = 0;
    for (int j = 0; j < MAX_TRANSACOES_BLOCO; j++) {
      if (curr->bloco.data[j * 3] == 0 && curr->bloco.data[j * 3 + 1] == 0 &&
          curr->bloco.data[j * 3 + 2] == 0) {
        break;
      }

      quantidade_transacao++;
      arr[i].quantidade = quantidade_transacao;
    }

    /* Guarda o bloco no vetor */
    arr[i].bloco = curr;

    /* Avanca para o proximo bloco, respeitando o limite n */
    curr = curr->prox;
  }

  ordernar_vetor(arr, n);

  for (int i = 0; i < n; i++)
    exibir_dados_bloco(*arr[i].bloco);
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

      /* Aqui eu minero direto e nao chamo a funcao de gerar transacoes */
      minerar_bloco(&bloco, hash, &est);

      ++est->array_quantidade_de_blocos_minerados[endereco_minerador];

      sistema->carteira[(uint32_t)bloco.data[DATA_LENGTH - 1]] +=
          RECOMPENSA_MINERACAO;

      inserir_enderecos_com_bitcoins(raiz, &est, endereco_minerador);

    } else {

      /* Depois de gerar as transacoes do bloco a funcao vai chamar a funcao
       * de minerar com os parametros corretos */

      /* A funcao de minerar é chamada dentro do gerar_transacoes, entao eu
       * gero transacao e minero o bloco
       * */
      gerar_transacoes_bloco(&bloco, sistema, &rand, raiz, &est, hash);

      sistema->carteira[(uint32_t)bloco.data[DATA_LENGTH - 1]] +=
          RECOMPENSA_MINERACAO;

      /* Verifica se ele tem apenas o valor da recompensa pois ele ainda nao
       * esta na lista, mas ja que recebeu bitcoin precisa inserir ele */
      if (sistema->carteira[(uint32_t)bloco.data[DATA_LENGTH - 1]] == 50)
        inserir_enderecos_com_bitcoins(raiz, &est,
                                       (uint32_t)bloco.data[DATA_LENGTH - 1]);
    }

    inserir_bloco(blockchain, &bloco, hash);

    FLAG_CHECA_CARTEIRAS = 0;

    while (FLAG_CHECA_CARTEIRAS != 1 &&
           (*est).tamanho_lista_enderecos_com_bitcoin > 1) {

      FLAG_CHECA_CARTEIRAS = checa_zero_bitcoin(raiz, &est, sistema);
    }

    /* Inicializa o data com 0 e copia o hash com base no outro hash */
    memset(bloco.data, 0, sizeof(bloco.data));
    memcpy(hash_anterior, hash, SHA256_DIGEST_LENGTH);
  }
}

void limpa_buffer() {
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;
}
