#include "blockchain.h"

int main() {
  struct enderecos_bitcoin *raiz = NULL;

  struct bloco_minerado *blockchain = NULL;

  /* A menor transacao nao pode ser zero na inicializacao pois sempre deve haver
   * alguem menor que 0 */
  struct estatisticas est = {.maior_transacao = 0,
                             .menor_transacao = MAX_TRANSACOES_BLOCO + 1,
                             .tamanho_lista_enderecos_com_bitcoin = 0,
                             .quantidade_total_bitcoins_por_bloco = 0};

  /* Preenche o array com 0 */
  memset(est.array_quantidade_de_blocos_minerados, 0,
         sizeof(est.array_quantidade_de_blocos_minerados));

  struct sistema_bitcoin sistema;

  iniciar_carteira(&sistema);
  processar_bloco(&raiz, &blockchain, &sistema, &est);

  /* Armazena os blocos de forma sequencial */
  insere_arquivo(&blockchain, "blocos.bin");

  /* Cria o arquivo indices.bin com base no endereco do minerador */
  criar_arquivo_indice("blocos.bin", "indices.bin");

  /* Cria o arquivo indices_nonce.bin com base nos nonces */
  criar_arquivo_indice_nonce("blocos.bin", "indices_nonce.bin");

  /* Funcao de teste para saber se os dados foram inseridos corretamente, no
   * caso passaria a struct para a funcao de imprimir */
  // struct bloco_minerado *test = le_arquivo("blocos.bin");

  /* Escrece os blocos em um txt */
  imprimir_blocos_minerados(blockchain, "blocos.txt");

  char opcao;

  while (1) {
    printf(
        "[a] => Endereco com mais bitcoins e o numero de bitcoins dele\n[b] "
        "=> Endereco que minerou mais blocos\n[c] "
        "=> Hash do bloco com mais transacoes e o numero de transacoes "
        "\n[d] => Hash com menos transacoes e o numero de transacoes\n[e] "
        "=> Quantidade media de bitcoins por bloco\n[f] => Imprimir todos "
        "os campos de um dado bloco\n[g] => Imprimir os n blocos "
        "minerados a partir de um endereco\n[h] => Imprimir os n primeiros "
        "blocos em ordem crescente de quantidade de transacoes\n[i] => "
        "Imprimir todos os "
        "blocos de um dado nonce\n[outro] => Sair\n"
        "----------------------------------------------------------------------"
        "--\n--> ");

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
      quantidade_media_bitcoins_bloco(est);
    else if (opcao == 'f')
      imprimir_bloco_arquivo_binario("blocos.bin");
    else if (opcao == 'g')
      imprimir_blocos_endereco("blocos.bin", "indices.bin");
    else if (opcao == 'i')
      imprimir_blocos_nonce("blocos.bin", "indices_nonce.bin");
    else if (opcao == 'h')
      transacoes_crescente(blockchain);
    else {
      printf("Deseja sair? (y / n): ");
      limpa_buffer();
      scanf("%c", &opcao);
      printf("\n\n");

      if (opcao == 'y')
        exit(0);
    }
  }

  free_blockchain(&blockchain);

  return EXIT_SUCCESS;
}
