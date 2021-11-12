#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

template <int min, int max>
int aleatorio()
{
    static default_random_engine generador((random_device())());
    static uniform_int_distribution<int> distribucion_uniforme(min, max);
    return distribucion_uniforme(generador);
}
/**************************************************************************/
int producir_ingrediente()
{
    // calcular milisegundos aleatorios de duración de la acción de fumar)
    chrono::milliseconds duracion_produ(aleatorio<10, 100>());

    // informa de que comienza a producir
    cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

    // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
    this_thread::sleep_for(duracion_produ);

    const int num_ingrediente = aleatorio<0, 2>();

    // informa de que ha terminado de producir
    cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

    return num_ingrediente;
}
/**************************************************************************/
void fumar(int num_fumador)
{

    // calcular milisegundos aleatorios de duración de la acción de fumar)
    chrono::milliseconds duracion_fumar(aleatorio<20, 200>());

    // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
         << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

    // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
    this_thread::sleep_for(duracion_fumar);

    // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
}
/**************************************************************************/
class Estanco : public HoareMonitor
{
private:
    static const int num_fumadores = 3;
    bool mostrador_vacio;
    int ingrediente_producido;
    CondVar estanquero;
    CondVar fumadores[num_fumadores];

public:
    Estanco();
    void ponerIngrediente(int ingrediente);
    void esperarRecogidaIngrediente();
    void obtenerIngrediente(int i);
};

Estanco::Estanco()
{
    mostrador_vacio = true;
    ingrediente_producido = -1;
    estanquero = newCondVar();

    for (int i = 0; i < num_fumadores; i++)
        fumadores[i] = newCondVar();
}

void Estanco::ponerIngrediente(int ingrediente)
{
    cout << "Estanquero : Pone ingrediente en el monitor" << endl;
    ingrediente_producido = ingrediente;
    mostrador_vacio = false;
    fumadores[ingrediente].signal();
}

void Estanco::esperarRecogidaIngrediente()
{
    if (!mostrador_vacio)
    {
        cout << "Estanquero : Espera recogida de ingrediente" << endl;
        estanquero.wait();
    }
}

void Estanco::obtenerIngrediente(int i)
{
    if (ingrediente_producido != i)
    {
        cout << "Fumador " << i << " : Espera por su ingrediente" << endl;
        fumadores[i].wait();
    }

    cout << "Fumador " << i << " : Coge ingrediente del mostrador" << endl;
    mostrador_vacio = true;
    estanquero.signal();
}

/**************************************************************************/
void funcion_hebra_estanquero(MRef<Estanco> monitor)
{
    int ingrediente;

    while (true)
    {
        ingrediente = producir_ingrediente();
        monitor->ponerIngrediente(ingrediente);
        monitor->esperarRecogidaIngrediente();
    }
}
/**************************************************************************/
void funcion_hebra_fumador(MRef<Estanco> monitor, int num_fumador)
{
    while (true)
    {
        monitor->obtenerIngrediente(num_fumador);
        fumar(num_fumador);
    }
}
/**************************************************************************/
int main()
{
    MRef<Estanco> monitor = Create<Estanco>();
    thread estanquero;
    thread fumadores[3];

    estanquero = thread(funcion_hebra_estanquero, monitor);

    for (int i = 0; i < 3; i++)
        fumadores[i] = thread(funcion_hebra_fumador, monitor, i);

    estanquero.join();

    for (int i = 0; i < 3; i++)
        fumadores[i].join();

    return 0;
}
