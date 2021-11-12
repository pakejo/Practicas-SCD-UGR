#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

const int num_escritores = 4;
const int num_lectores = 5;

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
    chrono::milliseconds duracion_lectura(aleatorio<400, 600>());
    this_thread::sleep_for(duracion_lectura);
}

void esperar_escritor()
{
    chrono::milliseconds duracion_escritura(aleatorio<100, 200>());
    this_thread::sleep_for(duracion_escritura);
}

void filtrando_aire()
{
    cout << "Filtrando el aire...." << endl;
    chrono::milliseconds duracion_filtrado(aleatorio<100, 200>());
    this_thread::sleep_for(duracion_filtrado);
    cout << "!!AIRE FILTRADO CON EXITO!!" << endl;
}

void ordenar_conserjeria()
{
    cout << "Conserje ordena la conserjeria" << endl;
    chrono::milliseconds duracion_filtrado(aleatorio<100, 200>());
    this_thread::sleep_for(duracion_filtrado);
    cout << "SILENCIO: por favor" << endl;
}

/*************************************************************************/
class Lec_Esc : public HoareMonitor
{
private:
    int n_lec;               // Numero de lectores leyendo
    bool escrib;             // True si hay algun escritor escribiendo
    int num_accesos_recurso; // Numero de accesos al recurso

    CondVar lectura;   // No hay escritores escribiendo, lectura posible
    CondVar escritura; // No hay lecturas ni escrituras, escritura posible
public:
    Lec_Esc();
    void ini_lectura();
    void fin_lectura();
    void ini_escritura();
    void fin_escritura();
    bool comprobar();
    void filtrado_completo();
};

Lec_Esc::Lec_Esc()
{
    n_lec = 0;
    escrib = false;
    num_accesos_recurso = 0;
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
    num_accesos_recurso++;
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
    num_accesos_recurso++;
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

bool Lec_Esc::comprobar()   // Examen
{
    bool hay_que_limpiar;

    if (num_accesos_recurso >= 10)
        hay_que_limpiar = true;
    else
        hay_que_limpiar = false;

    return hay_que_limpiar;
}

void Lec_Esc::filtrado_completo()   // Examen
{
    num_accesos_recurso = 0;
}

/*************************************************************************/
void funcion_hebra_escritor(MRef<Lec_Esc> monitor)
{
    while (true)
    {
        monitor->ini_escritura();
        esperar_escritor();
        monitor->fin_escritura();
        esperar_escritor();
    }
}
/*************************************************************************/
void funcion_hebra_lector(MRef<Lec_Esc> monitor)
{
    while (true)
    {
        monitor->ini_lectura();
        esperar_lector();
        monitor->fin_lectura();
        esperar_lector();
    }
}
/*************************************************************************/
void funcion_hebra_conserje(MRef<Lec_Esc> monitor)  // Examen
{
    bool hacer_filtrado;

    while (true)
    {
        hacer_filtrado = monitor->comprobar();

        if (hacer_filtrado)
        {
            filtrando_aire();
            monitor->filtrado_completo();
        }
        else
            ordenar_conserjeria();
    }
}
/*************************************************************************/

int main(int argc, char const *argv[])
{
    MRef<Lec_Esc> monitor = Create<Lec_Esc>();
    thread escritores[num_escritores];
    thread lectores[num_lectores];
    thread conserje;

    conserje = thread(funcion_hebra_conserje, monitor);

    for (int i = 0; i < num_escritores; i++)
        escritores[i] = thread(funcion_hebra_escritor, monitor);

    for (int i = 0; i < num_lectores; i++)
        lectores[i] = thread(funcion_hebra_lector, monitor);

    conserje.join();

    for (int i = 0; i < num_escritores; i++)
        escritores[i].join();

    for (int i = 0; i < num_lectores; i++)
        lectores[i].join();

    return 0;
}
