#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

/*************************************************************************/
template <int min, int max>
int aleatorio()
{
    static default_random_engine generador((random_device())());
    static uniform_int_distribution<int> distribucion_uniforme(min, max);
    return distribucion_uniforme(generador);
}

void esperar_lector()
{
    // calcular milisegundos aleatorios de duración de la acción de fumar)
    chrono::milliseconds duracion_produ(aleatorio<400, 600>());

    // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
    this_thread::sleep_for(duracion_produ);
}

void esperar_escritor()
{
    // calcular milisegundos aleatorios de duración de la acción de fumar)
    chrono::milliseconds duracion_produ(aleatorio<100, 200>());

    // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
    this_thread::sleep_for(duracion_produ);
}

/*************************************************************************/
class Lec_Esc : public HoareMonitor
{
private:
    int n_lec;         // Numero de lectores leyendo
    bool escrib;       // True si hay algun escritor escribiendo
    CondVar lectura;   // No hay escritores escribiendo, lectura posible
    CondVar escritura; // No hay lecturas ni escrituras, escritura posible
public:
    Lec_Esc();
    void ini_lectura();
    void fin_lectura();
    void ini_escritura();
    void fin_escritura();
};

Lec_Esc::Lec_Esc()
{
    n_lec = 0;
    escrib = false;
    lectura = newCondVar();
    escritura = newCondVar();
}

void Lec_Esc::ini_lectura()
{
    cout << "Lector inicia lectura" << endl;

    if (escrib)
    {
        cout << "Hay un escritor escribiendo, lector espera" << endl;
        lectura.wait();
    }

    n_lec++;
    cout << "Numero de lectores leyendo: " << n_lec << endl;
    lectura.signal();
}

void Lec_Esc::fin_lectura()
{
    cout << "Lector termina de leer" << endl;
    n_lec--;
    cout << "Numero de lectores leyendo: " << n_lec << endl;

    if (n_lec == 0)
    {
        cout << "No hay lectores en espera. Notificando a escritores" << endl;
        escritura.signal();
    }
}

void Lec_Esc::ini_escritura()
{
    cout << "Escritor inicia escritura" << endl;

    if (n_lec > 0 || escrib)
    {
        cout << "Hay otro escritor escribiendo o lectores leyendo, escritor espera" << endl;
        escritura.wait();
    }

    escrib = true;
}

void Lec_Esc::fin_escritura()
{
    cout << "Escritor termina escritura" << endl;
    escrib = false;

    if (!lectura.empty())
    {
        cout << "Hay lectores en espera. Notificando a lectores" << endl;
        lectura.signal();
    }
    else
    {
        cout << "No hay lectores en espera. Notificando a escritorres" << endl;
        escritura.signal();
    }
}

/*************************************************************************/
void funcion_hebra_escritor(MRef<Lec_Esc> monitor)
{
    while (true)
    {
        monitor->ini_escritura();
        esperar_escritor();   // Provisional
        monitor->fin_escritura();
        esperar_escritor();   // Provisional
    }
}
/*************************************************************************/
void funcion_hebra_lector(MRef<Lec_Esc> monitor)
{
    while (true)
    {
        monitor->ini_lectura();
        esperar_lector();  // Provisional
        monitor->fin_lectura();
        esperar_lector();   // Provisional
    }
}
/*************************************************************************/

int main(int argc, char const *argv[])
{
    const int num_escritores = 8;
    const int num_lectores = 10;

    MRef<Lec_Esc> monitor = Create<Lec_Esc>();
    thread escritores[num_escritores];
    thread lectores[num_lectores];

    for (int i = 0; i < num_escritores; i++)
        escritores[i] = thread(funcion_hebra_escritor, monitor);

    for (int i = 0; i < num_lectores; i++)
        lectores[i] = thread(funcion_hebra_lector, monitor);

    for (int i = 0; i < num_escritores; i++)
        escritores[i].join();

    for (int i = 0; i < num_lectores; i++)
        lectores[i].join();

    return 0;
}
