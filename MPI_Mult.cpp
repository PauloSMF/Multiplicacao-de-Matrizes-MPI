#include "mpi.h"
#include<iostream>
#include<cstdlib>
#include<ctime>

using namespace std;

int main(int argc, char *argv[]){
    setlocale(LC_ALL,"Portuguese");
    int rank, nprocs;   //Identificador de processo e número de processos
    int l = 2;  //Valor padrão para linhas da matriz A e C
    int m = 3;  //Valor padrão para colunas da matriz A e linhas da matriz B
    int n = 2;  //Valor padrão para colunas da matriz B e C
    clock_t inicio;

    MPI_Init(&argc, &argv); //Definindo e inicializando o ambiente necesserário para executar o MPI
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs); //Verificando o número de processos
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);   //Identificando o processo

    if(rank == 0){
        if(argc == 4){   //Caso o usuário tenha informado o número n
            l = stoi(argv[1]);  //Guardando l informado pelo usuário
            m = stoi(argv[2]);  //Guardando m informado pelo usuário
            n = stoi(argv[3]);  //Guardando n informado pelo usuário
            //Iniciando a contagem de tempo
            inicio = clock();
        }
    }
    //Enviando l, m, n para os outros processos
    MPI_Bcast(&l, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //--- Instanciando as matrizes ---//
    //Matriz A
    int A[l][m];
    //Matriz B
    int B[m][n];
    //Matriz C
    int C[l][n];

    //Armazenanado quantas e a partir de qual linha cada processo
    int linhasCalculadas;   //Realiza o cálculo da multiplicação
    int comecandoDaLinha;
    int nProcessosMaisLinhas;   //Armazena quantos processos calculam mais linhas

    //Calculando quantos processos recebem mais linhas
    nProcessosMaisLinhas = l%nprocs;
    //Calculando quantas linhas o processo realiza a multiplicação
    if(rank < nProcessosMaisLinhas)
        linhasCalculadas = l/nprocs+1;
    else
        linhasCalculadas = l/nprocs;

    //cout<<"Processo : "<<rank<<" l = "<<l<<" m = "<<m<<" n = "<<n<<endl;
    //cout<<"Linhas calc processo: "<<rank<<": "<<linhasCalculadas<<endl;
    //cout<<"nProcessosMaisLinhas= "<<nProcessosMaisLinhas<<endl;

    if(rank == 0){  //Processo 0 preenche as matrizes com números aleatórios
        srand((int)time(0));
        //--- Preenchimento das matrizes ---//
        //Preenchendo uma matriz A pré-estabelecida
        for (int i = 0; i < l; i++)
            for (int j = 0; j < m; j++)
                A[i][j] = (rand() % 10) + 1;

        //Preenchendo uma matriz B pré-estabelecida
        for (int i = 0; i < m; i++)
            for (int j = 0; j < n; j++)
                B[i][j] = (rand() % 10) + 1;

        //Preenchendo uma matriz C com 0s
        for (int i = 0; i < l; i++)
            for (int j = 0; j < n; j++)
                C[i][j] = 0;

        //--- Impressão das matrizes A e B---//
        cout<<"Matriz A: "<<endl;
        for (int i = 0; i < l; i++){
            for (int j = 0; j < m; j++)
                cout<<A[i][j]<<" ";
            cout<<endl;
        }
        cout<<endl;
        cout<<"Matriz B: "<<endl;
        for (int i = 0; i < m; i++){
            for (int j = 0; j < n; j++)
                cout<<B[i][j]<<" ";
            cout<<endl;
        }
        cout<<endl;

        //--- Calculo do deslocamento de linhas ---//
        comecandoDaLinha = 0;

        for(int processo = 1; processo < nprocs; processo++){
            comecandoDaLinha = comecandoDaLinha + linhasCalculadas;  //Calculando deslocamento para o processo
            //Calculando as linhas calculadas pelo processo
            if(processo < nProcessosMaisLinhas)
                linhasCalculadas = l/nprocs+1;
            else
                linhasCalculadas = l/nprocs;
            //Enviando o deslocamento de linhas e as linhas de cada processo
            MPI_Send(&comecandoDaLinha, 1, MPI_INT, processo, 0, MPI_COMM_WORLD);
            MPI_Send(&A[comecandoDaLinha][0], linhasCalculadas*m, MPI_INT, processo, 0, MPI_COMM_WORLD);
        }

        //Resetando informações para o processo 0
        comecandoDaLinha = 0;
        if(rank < nProcessosMaisLinhas)
            linhasCalculadas = l/nprocs+1;
        else
            linhasCalculadas = l/nprocs;
    }
    else{   //Recebendo as informações do processo 0
        MPI_Recv(&comecandoDaLinha, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&A, linhasCalculadas*m, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    //Enviando a matriz B preenchida pelo processo 0 para outros processos
    MPI_Bcast(&B, m*n, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&C, l*n, MPI_INT, 0, MPI_COMM_WORLD);

    //Multiplicando as linhas atribuidas ao processo por colunas da matriz B
    for(int i = 0; i < linhasCalculadas; i++)
        for(int j = 0; j < n; j++){
            for(int k = 0; k < m; k++)
                C[i][j] = C[i][j] + (A[i][k]* B[k][j]);

            //cout<<"Rank "<<rank<<" multiplica linha "<<i+comecandoDaLinha<<" da matriz A por coluna "
                //<<j<<" da matriz B e o resultado foi: "<<C[i][j]<<endl;
        }

    if(rank != 0){  //Os processos enviam as informações para o processo 0
        MPI_Send(&linhasCalculadas, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);  //As linhas que calculou
        MPI_Send(&comecandoDaLinha, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);  //Por qual começou
        MPI_Send(&C, linhasCalculadas*n, MPI_INT, 0, 1, MPI_COMM_WORLD);    //E o resultado
    }
    else{   //Processo 0 recebe informações dos outros
        for(int processo = 1; processo < nprocs; processo++){
            MPI_Recv(&linhasCalculadas, 1, MPI_INT, processo, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&comecandoDaLinha, 1, MPI_INT, processo, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&C[comecandoDaLinha][0], l*n, MPI_INT, processo, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }   //A partir destes dados a matriz C é atualizada
        //Cálculo de tempo
        double tempoFinal = (clock() - inicio) / (double)CLOCKS_PER_SEC;
        //--- Impressão da matriz C ---//
        cout<<"Matriz C: "<<endl;
        for (int i = 0; i < l; i++){
            for (int j = 0; j < n; j++)
                cout<<C[i][j]<<" ";
            cout<<endl;
        }
        cout<<endl;
        printf("\nTempo total: %f segundos\n", tempoFinal);
    }

    MPI_Finalize(); //Finalizando o processo MPI
    return 0;
}
