#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std;
using namespace SEM;

const int num_fumadores = 4;
Semaphore puede_producir = 1;
Semaphore puede_retirar[num_fumadores] = {0, 0, 0, 0}; // Fumador pipa es el ultimo (3)
int num_veces_fumador_0 = 0;                           // Numero de veces que fuma el fumador 0

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

   const int num_ingrediente = aleatorio<0, num_fumadores - 2>();

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
      cout << "Estanquero : produce ingrediente " << i << endl
           << flush;
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

      if (num_fumador == 0 && num_veces_fumador_0 == 4)
      {
         cout << "El fumador 0 ha fumado 4 veces. Notificando a fumador de pipa" << endl;
         sem_signal(puede_retirar[3]);         // Avisamos al fumador de pipa que puede fumar
         sem_wait(puede_retirar[num_fumador]); // El fumador 0 espera bloqueado
      }

      if (num_fumador == 0)
      {
         num_veces_fumador_0++;
         cout << "N: " << num_veces_fumador_0 << endl;
      }

      cout << "Fumador " << num_fumador << "  : retira su ingrediente" << endl;

      sem_signal(puede_producir);
      fumar(num_fumador);
   }
}

void funcion_hebra_fumador_pipa()
{
   sem_wait(puede_retirar[3]);
   num_veces_fumador_0 = 0;
   cout << "Fumador de pipa empieza a fumar" << endl;
   fumar(3);
   cout << "Fumador de pipa termina de fumar. Despierta fumador 0" << endl;
   sem_signal(puede_retirar[0]);
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   thread estanquero = thread(funcion_hebra_estanquero);
   thread fumador_0(funcion_hebra_fumador, 0);
   thread fumador_1(funcion_hebra_fumador, 1);
   thread fumador_2(funcion_hebra_fumador, 2);
   thread fumador_pipa(funcion_hebra_fumador_pipa);

   estanquero.join();
   fumador_0.join();
   fumador_1.join();
   fumador_2.join();
   fumador_pipa.join();

   return 0;
}
