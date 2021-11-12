#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std;
using namespace SEM;

const int num_fumadores = 3;
Semaphore puede_producir = 1;
Semaphore puede_retirar[num_fumadores] = {0, 0, 0};

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

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ(aleatorio<10, 100>());

   // informa de que comienza a producir
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for(duracion_produ);

   const int num_ingrediente = aleatorio<0, num_fumadores - 1>();

   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

   return num_ingrediente;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero()
{
   while (true)
   {
      int i = producir_ingrediente();
      sem_wait(puede_producir);
      cout << "Estanquero : produce ingrediente " << i << endl << flush;
      sem_signal(puede_retirar[i]);
   }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

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

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void funcion_hebra_fumador(int num_fumador)
{
   while (true)
   {
      sem_wait(puede_retirar[num_fumador]);
      cout << "Fumador " << num_fumador << "  : retira su ingrediente" << endl <<flush;
      sem_signal(puede_producir);
      fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   thread estanquero = thread(funcion_hebra_estanquero);
   thread fumador_0(funcion_hebra_fumador, 0);
   thread fumador_1(funcion_hebra_fumador, 1);
   thread fumador_2(funcion_hebra_fumador, 2);

   estanquero.join();
   fumador_0.join();
   fumador_1.join();
   fumador_2.join();

   return 0;
}
