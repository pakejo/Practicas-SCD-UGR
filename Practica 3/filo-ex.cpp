// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-plantilla.cpp
// Implementación del problema de los filósofos (con camarero).
// Plantilla para completar.
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------

#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

#define etiq_filosofo 1
#define etiq_sentarse 2
#define etiq_levantarse 3

const int
    id_camarero = 10,
    num_filosofos = 5,
    num_procesos = 2 * num_filosofos + 1;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template <int min, int max>
int aleatorio()
{
    static default_random_engine generador((random_device())());
    static uniform_int_distribution<int> distribucion_uniforme(min, max);
    return distribucion_uniforme(generador);
}

// ---------------------------------------------------------------------

void funcion_filosofos(int id)
{
    int id_ten_izq = (id + 1) % (num_procesos - 1),                //id. tenedor izq.
        id_ten_der = (id + num_procesos - 2) % (num_procesos - 1), //id. tenedor der.
        peticion = 1;
    MPI_Status estado;

    if (id == 6)
        peticion = 2;

    while (true)
    {
        cout << "Filosofo " << id << " solicita sentarse" << endl;
        MPI_Send(&peticion, 1, MPI_INT, id_camarero, etiq_sentarse, MPI_COMM_WORLD);

        MPI_Recv(&peticion, 1, MPI_INT, id_camarero, etiq_sentarse, MPI_COMM_WORLD, &estado);
        cout << "Filosofo " << id << " se sienta" << endl;

        cout << "Filósofo " << id << " solicita ten. izq." << id_ten_izq << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, etiq_filosofo, MPI_COMM_WORLD);

        cout << "Filósofo " << id << " solicita ten. der." << id_ten_der << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, etiq_filosofo, MPI_COMM_WORLD);

        cout << "Filósofo " << id << " comienza a comer" << endl;
        sleep_for(milliseconds(aleatorio<10, 100>()));

        cout << "Filósofo " << id << " suelta ten. izq. " << id_ten_izq << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, etiq_filosofo, MPI_COMM_WORLD);

        cout << "Filósofo " << id << " suelta ten. der. " << id_ten_der << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, etiq_filosofo, MPI_COMM_WORLD);

        cout << "Filósofo " << id << " se levanta " << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, etiq_levantarse, MPI_COMM_WORLD);

        cout << "Filosofo " << id << " comienza a pensar" << endl;
        sleep_for(milliseconds(aleatorio<10, 100>()));
    }
}
// ---------------------------------------------------------------------

void funcion_tenedores(int id)
{
    int valor, id_filosofo; // valor recibido, identificador del filósofo
    MPI_Status estado;      // metadatos de las dos recepciones

    while (true)
    {
        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_filosofo, MPI_COMM_WORLD, &estado);
        id_filosofo = estado.MPI_SOURCE;
        cout << "Ten. " << id << " ha sido cogido por filo. " << id_filosofo << endl;

        MPI_Recv(&valor, 1, MPI_INT, id_filosofo, etiq_filosofo, MPI_COMM_WORLD, &estado);
        cout << "Ten. " << id << " ha sido liberado por filo. " << id_filosofo << endl;
    }
}

void funcion_camarero()
{
    int num_filosofos_sentados = 0;
    int num_sillas_ocupadas = 0;
    int peticion_filosofo;

    MPI_Status status;

    while (true)
    {
        if (num_filosofos_sentados < 4)
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        else
            MPI_Probe(MPI_ANY_SOURCE, etiq_levantarse, MPI_COMM_WORLD, &status);

        switch (status.MPI_TAG)
        {
        case etiq_sentarse:
            // Recibimos peticion para sentarse
            MPI_Recv(&peticion_filosofo, 1, MPI_INT, status.MPI_SOURCE, etiq_sentarse, MPI_COMM_WORLD, &status);

            // Incrementamos contador de filosofos sentados
            num_filosofos_sentados++;

            num_sillas_ocupadas += peticion_filosofo;

            // Enviamos confirmacion
            MPI_Ssend(&peticion_filosofo, 1, MPI_INT, status.MPI_SOURCE, etiq_sentarse, MPI_COMM_WORLD);

            cout << "Número de filósofos sentados: " << num_filosofos_sentados << endl;
            cout << "Numero de sillas ocupadas: " << num_sillas_ocupadas << endl;
            break;

        case etiq_levantarse:
            // Recibimos peticion para levantarse
            MPI_Recv(&peticion_filosofo, 1, MPI_INT, status.MPI_SOURCE, etiq_levantarse, MPI_COMM_WORLD, &status);

            // Decrementamos contador de filosofos sentados
            num_filosofos_sentados--;

            num_sillas_ocupadas -= peticion_filosofo;

            cout << "Número de filósofos sentados: " << num_filosofos_sentados << endl;
            cout << "Numero de sillas ocupadas: " << num_sillas_ocupadas << endl;
            break;
        }
    }
}

// ---------------------------------------------------------------------

int main(int argc, char **argv)
{
    int id_propio, num_procesos_actual;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

    if (num_procesos == num_procesos_actual)
    {
        // ejecutar la función correspondiente a 'id_propio'
        if (id_propio == 10)
            funcion_camarero();
        else if (id_propio % 2 == 0)      // si es par
            funcion_filosofos(id_propio); //   es un filósofo
        else                              // si es impar
            funcion_tenedores(id_propio); //   es un tenedor
    }
    else
    {
        if (id_propio == 0) // solo el primero escribe error, indep. del rol
        {
            cout << "el número de procesos esperados es:    " << num_procesos << endl
                 << "el número de procesos en ejecución es: " << num_procesos_actual << endl
                 << "(programa abortado)" << endl;
        }
    }

    MPI_Finalize();
    return 0;
}