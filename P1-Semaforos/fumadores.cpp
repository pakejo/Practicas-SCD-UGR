#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <string>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//**********************************************************************
// variables compartidas

const int num_fumadores = 3;
Semaphore ingr_dis[num_fumadores] = {0, 0, 0};
Semaphore mostr_vacio = 1;
string ingredientes[3] = {"cerillas", "tabaco", "papel"};
mutex mtx;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
  int ingrediente;

  while (true)
  {
    ingrediente = aleatorio<0,2>();
    sem_wait(mostr_vacio);
    
    mtx.lock();
    cout << "Producido el ingrediente: " <<ingredientes[ingrediente] << endl;
    mtx.unlock();

    sem_signal(ingr_dis[ingrediente]);
  }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,1500>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
  while (true)
  {
    sem_wait(ingr_dis[num_fumador]);

    cout << "Retirando " << ingredientes[num_fumador] << " de la mesa." << endl;

    sem_signal(mostr_vacio);

    mtx.lock();
    fumar(num_fumador);
    mtx.unlock();
  }
}

//----------------------------------------------------------------------

int main()
{
  thread estanquero(funcion_hebra_estanquero);
  thread fumador_0(funcion_hebra_fumador, 0);
  thread fumador_1(funcion_hebra_fumador, 1);
  thread fumador_2(funcion_hebra_fumador, 2);

  estanquero.join();
  fumador_0.join();
  fumador_1.join();
  fumador_2.join();
}
