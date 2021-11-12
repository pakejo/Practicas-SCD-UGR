#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

const int num_procesos_mostrador = 5;
const int num_procesos_clientes = 11;
const int id_proceso_controlador = 16;
const int num_procesos = 17;

const int etiq_SOLICITAR_MOSTRADOR = 0;
const int etiq_ENTRAR_MOSTRADOR = 1;
const int etiq_SALIR_MOSTRADOR = 2;
const int etiq_SERVICIO_COMPLETADO = 3;

template <int min, int max>
int aleatorio()
{
    static default_random_engine generador((random_device())());
    static uniform_int_distribution<int> distribucion_uniforme(min, max);
    return distribucion_uniforme(generador);
}

void funcion_mostrador()
{
    int peticion = 0;
    int cliente;
    MPI_Status status;

    MPI_Recv(&peticion, 1, MPI_INT, MPI_ANY_SOURCE, etiq_ENTRAR_MOSTRADOR, MPI_COMM_WORLD, &status);

    cliente = status.MPI_SOURCE;
    sleep_for(milliseconds(aleatorio<10, 100>()));

    MPI_Ssend(&peticion, 1, MPI_INT, cliente, etiq_SALIR_MOSTRADOR, MPI_COMM_WORLD);
}

void funcion_cliente(int id_propio)
{
    int peticion = 0;
    int num_mostrador;
    MPI_Status status;

    while (true)
    {
        cout << "Cliente " << id_propio << " solicita mostrador" << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_proceso_controlador, etiq_SOLICITAR_MOSTRADOR, MPI_COMM_WORLD);
        MPI_Recv(&num_mostrador, 1, MPI_INT, id_proceso_controlador, etiq_SOLICITAR_MOSTRADOR, MPI_COMM_WORLD, &status);

        cout << "Cliente " << id_propio << " se dirige al mostrador:" << num_mostrador << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, num_mostrador, etiq_ENTRAR_MOSTRADOR, MPI_COMM_WORLD);
        MPI_Recv(&peticion, 1, MPI_INT, num_mostrador, etiq_SALIR_MOSTRADOR, MPI_COMM_WORLD, &status);
        cout << "Cliente " << id_propio << ". Servicio completado" << endl;

        cout << "Informando al controlador de que esta libre el mostrador" << endl;
        MPI_Ssend(&num_mostrador, 1, MPI_INT, id_proceso_controlador, etiq_SERVICIO_COMPLETADO, MPI_COMM_WORLD);
    }
}

void funcion_controlador()
{
    bool esta_libre_mostrador[num_procesos_mostrador] = {true, true, true, true, true};
    int num_mostradores_libres = 0;
    int peticion;
    int mostrador;
    bool para = false;
    MPI_Status status;

    while (true)
    {
        if (num_mostradores_libres < 5)
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        else
            MPI_Probe(MPI_ANY_SOURCE, etiq_SERVICIO_COMPLETADO, MPI_COMM_WORLD, &status);

        switch (status.MPI_TAG)
        {
        case etiq_SOLICITAR_MOSTRADOR:
            // Recibimos la peticion
            MPI_Recv(&peticion, 1, MPI_INT, status.MPI_SOURCE, etiq_SOLICITAR_MOSTRADOR, MPI_COMM_WORLD, &status);

            // Buscamos un mostrador libre
            for (int i = 0; i < num_procesos_mostrador && !para; i++)
            {
                if (esta_libre_mostrador[i])
                {
                    mostrador = i;
                    para = true;
                }

                esta_libre_mostrador[i] = false;
                para = false;
            }

            num_mostradores_libres--;

            // Enviamos el mostrador libre
            MPI_Ssend(&mostrador, 1, MPI_INT, status.MPI_SOURCE, etiq_SOLICITAR_MOSTRADOR, MPI_COMM_WORLD);
            break;

        case etiq_SERVICIO_COMPLETADO:
            MPI_Recv(&peticion, 1, MPI_INT, status.MPI_SOURCE, etiq_SERVICIO_COMPLETADO, MPI_COMM_WORLD, &status);
            esta_libre_mostrador[peticion] = true;
            num_mostradores_libres++;
            break;
        }
    }
}

int main(int argc, char **argv)
{
    int id_propio, num_procesos_actual;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

    if (num_procesos == num_procesos_actual)
    {
        // ejecutar la función correspondiente a 'id_propio'
        if (id_propio < 5)
            funcion_mostrador(); // Si es menor que 5 es un mostrador
        else if ((id_propio >= 5) && (id_propio < 16))
            funcion_cliente(id_propio); // entre 5 y 15 es un cliente
        else
            funcion_controlador(); //   es el controlador
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