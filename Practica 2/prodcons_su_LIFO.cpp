#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

constexpr int
    num_items = 40, // número de items a producir/consumir
    num_prod = 4,   // número de productores
    num_cons = 4;   // número de consumidores

unsigned
    cont_prod[num_items], // contadores de verificación: producidos
    cont_cons[num_items]; // contadores de verificación: consumidos

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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato()
{
    static int contador = 0;
    this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));
    cont_prod[contador]++;
    return contador++;
}
//----------------------------------------------------------------------

void consumir_dato(unsigned dato)
{
    if (num_items <= dato)
    {
        cout << " dato === " << dato << ", num_items == " << num_items << endl;
        assert(dato < num_items);
    }
    cont_cons[dato]++;
    this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));
}
//----------------------------------------------------------------------

void ini_contadores()
{
    for (unsigned i = 0; i < num_items; i++)
    {
        cont_prod[i] = 0;
        cont_cons[i] = 0;
    }
}

//----------------------------------------------------------------------

void test_contadores()
{
    bool ok = true;
    cout << "comprobando contadores ...." << flush;

    for (unsigned i = 0; i < num_items; i++)
    {
        if (cont_prod[i] != 1)
        {
            cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl;
            ok = false;
        }
        if (cont_cons[i] != 1)
        {
            cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl;
            ok = false;
        }
    }
    if (ok)
        cout << endl
             << flush << "solución (aparentemente) correcta." << endl
             << flush;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SU, varios prod. y varios cons.

class ProdConsSU : public HoareMonitor
{
private:
    static const int num_celdas_total = 10; // núm. de entradas del buffer
    int buffer[num_celdas_total];           // buffer para guardar valores
    int primera_libre;                      // Primera posicion libre del buffer
    CondVar cola_productores;               // Cola de productores
    CondVar cola_consumidores;              // Cola de consumidores

public:
    ProdConsSU();
    void escribir(int valor);
    int leer();
};

ProdConsSU::ProdConsSU()
{
    primera_libre = 0;
    cola_productores = newCondVar();
    cola_consumidores = newCondVar();
}

void ProdConsSU::escribir(int valor)
{
    if (primera_libre == num_celdas_total)
        cola_productores.wait();

    buffer[primera_libre] = valor;
    primera_libre++;

    cola_consumidores.signal();
}

int ProdConsSU::leer()
{
    if (primera_libre == 0)
        cola_consumidores.wait();

    int valor = buffer[primera_libre - 1];
    primera_libre--;

    cola_productores.signal();

    return valor;
}

// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora(MRef<ProdConsSU> monitor, int id)
{
    for (unsigned i = id; i < num_items; i += num_prod)
    {
        int valor = producir_dato();
        cout << "Productor " << id << " produce valor: " << valor << endl;
        monitor->escribir(valor);
    }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora(MRef<ProdConsSU> monitor, int id)
{
    for (unsigned i = id; i < num_items; i += num_cons)
    {
        int valor = monitor->leer();
        cout << "Consumidor " << id << " consume valor: " << valor << endl;
        consumir_dato(valor);
    }
}
// -----------------------------------------------------------------------------

int main()
{
    cout << "--------------------------------------------------------------------------------------" << endl
         << "Problema de los productores-consumidores (varios prod/cons, Monitor SU, buffer LIFO). " << endl
         << "--------------------------------------------------------------------------------------" << endl
         << flush;

    MRef<ProdConsSU> monitor = Create<ProdConsSU>();
    thread productores[num_prod];
    thread consumidores[num_cons];

    for (int i = 0; i < 4; i++)
    {
        productores[i] = thread(funcion_hebra_productora, monitor, i);
        consumidores[i] = thread(funcion_hebra_consumidora, monitor, i);
    }

    for (int i = 0; i < 4; i++)
    {
        productores[i].join();
        consumidores[i].join();
    }

    // comprobar que cada item se ha producido y consumido exactamente una vez
    test_contadores();
}