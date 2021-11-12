#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

const int num_hebra_tipo_1 = 7;
const int num_hebra_tipo_2 = 4;

//**********************************************************************
template <int min, int max>
int aleatorio()
{
    static default_random_engine generador((random_device())());
    static uniform_int_distribution<int> distribucion_uniforme(min, max);
    return distribucion_uniforme(generador);
}

void esperar_preparacion()
{
    this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));
}

void irse()
{
    this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));
}
//**********************************************************************
class Comida : public HoareMonitor
{
private:
    static const int num_maquina_tipo_1 = 4;    // numero de maquinas de tipo 1
    static const int num_maquina_tipo_2 = 3;    // numero de maquinas de tipo 2

    int num_maquinas_tipo_1_uso;                // numero de maquinas de tipo 1 en uso
    int num_maquinas_tipo_2_uso;                // numero de maquinas de tipo 2 en uso

    CondVar cola_maquina_tipo_1;                // cola para comensales tipo 1
    CondVar cola_maquina_tipo_2;                // cola para comensales tipo 2

public:
    Comida();
    void pedir_bocata(int tipo_bocata);
    void bocata_servido(int tipo_bocata);
};

Comida::Comida(/* args */)
{
    num_maquinas_tipo_1_uso = 0;
    num_maquinas_tipo_2_uso = 0;
    cola_maquina_tipo_1 = newCondVar();
    cola_maquina_tipo_2 = newCondVar();
}

void Comida::pedir_bocata(int tipo_bocata)
{
    if (tipo_bocata == 1) // maquina tipo 1
    {
        if (num_maquinas_tipo_1_uso == num_maquina_tipo_1)
            cola_maquina_tipo_1.wait();
        else
            num_maquinas_tipo_1_uso++;
    }

    if (tipo_bocata == 2) // maquina tipo 1
    {
        if (num_maquinas_tipo_2_uso == num_maquina_tipo_2)
            cola_maquina_tipo_2.wait();
        else
            num_maquinas_tipo_2_uso++;
    }
}

void Comida::bocata_servido(int tipo_bocata)
{
    if (tipo_bocata == 1)
    {
        num_maquinas_tipo_1_uso--;
        cola_maquina_tipo_1.signal();
    }
    else
    {
        num_maquinas_tipo_2_uso--;
        cola_maquina_tipo_2.signal();
    }

    cout << "num maquinas tipo 1 uso: " << num_maquinas_tipo_1_uso <<endl;
    cout << "num maquinas tipo 2 uso: " << num_maquinas_tipo_2_uso <<endl;
}

//**********************************************************************
void funcion_hebra_alumno(MRef<Comida> comida, int tipo_comida)
{
    while (true)
    {
        comida->pedir_bocata(tipo_comida);
        cout << "Hebra espera preparacion de comida" << endl;
        esperar_preparacion();
        cout << "Hebra recibe comida. Se marcha" << endl;
        comida->bocata_servido(tipo_comida);
    }
}
//**********************************************************************
int main()
{
    MRef<Comida> monitor = Create<Comida>();
    thread comensales_tipo_1[num_hebra_tipo_1];
    thread comensales_tipo_2[num_hebra_tipo_2];

    for (int i = 0; i < num_hebra_tipo_1; i++)
        comensales_tipo_1[i] = thread(funcion_hebra_alumno, monitor, 1);

    for (int i = 0; i < num_hebra_tipo_2; i++)
        comensales_tipo_2[i] = thread(funcion_hebra_alumno, monitor, 2);

    for (int i = 0; i < num_hebra_tipo_1; i++)
        comensales_tipo_1[i].join();

    for (int i = 0; i < num_hebra_tipo_2; i++)
        comensales_tipo_2[i].join();

    return 0;
}
