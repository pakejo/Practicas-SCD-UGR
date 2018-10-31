#include <iostream>
#include <string>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.hpp"

                   using namespace std;
using namespace HM;

const unsigned num_fumadores = 3;
string vector_ingredientes[3] = {"cerillas", "tabaco", "papel"};

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
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar(int num_fumador)
{
  // calcular milisegundos aleatorios de duración de la acción de fumar)
  chrono::milliseconds duracion_fumar(aleatorio<20, 200>());

  // informa de que comienza a fumar
  cout << "Fumador " << num_fumador << "  :"
       << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

  // espera bloqueada un tiempo igual a 'duracion_fumar' milisegundos
  this_thread::sleep_for(duracion_fumar);

  // informa de que ha terminado de fumar
  cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
}

int producir()
{
  // calcular milisegundos aleatorios de duración de la acción de producir)
  chrono::milliseconds duracion_producir(aleatorio<20, 200>());

  // informa de que comienza a producir

  cout << "Estanquero: "
       << " empieza a producir (" << duracion_producir.count() << " milisegundos)" << endl;

  // espera bloqueada un tiempo igual a ''duracion_producir' milisegundos
  this_thread::sleep_for(duracion_producir);
  int ingrediente = aleatorio<0, 2>();

  // informa de que ha terminado de producir

  cout << "Estanquero produce ingrediente: " << vector_ingredientes[ingrediente] << endl;
  return ingrediente;
}

class Estanco : public HoareMonitor
{
private:
  int ingrediente; //Ingrediente necesario para el fumador
  bool libre;      //bool que indica si el mostrador esta libre

  CondVar
      estanquero,               //Cola condicion para el estanquero
      fumadores[num_fumadores]; //Cola condicion para los fumadores

public:
  Estanco();
  void obtenerIngrediente(int indice);
  void ponerIngrediente(int indice);
  void esperaIngrediente();
};

Estanco::Estanco()
{
  ingrediente = -1;
  libre = true;

  estanquero = newCondVar();
  for (int i = 0; i < num_fumadores; i++)
    fumadores[i] = newCondVar();
}

void Estanco::obtenerIngrediente(int indice)
{
  //Si el mostrador esta libre los fumadores esperan
  if (libre)
    fumadores[indice].wait();

  //Si el ingrediente del mostrador no es el que le corresponde espera
  if (ingrediente != indice)
    fumadores[indice].wait();

  cout << "Retirando ingrediente: " << vector_ingredientes[ingrediente] << endl;
  libre = true;

  //Manda señal al estanquero para que ponga un ingrediente
  estanquero.signal();
}

void Estanco::ponerIngrediente(int indice)
{
  cout << "Puesto ingrediente: " << vector_ingredientes[indice] << endl;
  ingrediente = indice;
  libre = false;
  fumadores[indice].signal();
}

void Estanco::esperaIngrediente()
{
  if (!libre)
  {
    cout << "Esperando recogida de ingrediente " << vector_ingredientes[ingrediente] << endl;
    estanquero.wait();
  }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void funcion_hebra_fumador(int num_fumador, MRef<Estanco> estanco)
{
  while (true)
  {
    estanco->obtenerIngrediente(num_fumador);
    fumar(num_fumador);
  }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(MRef<Estanco> estanco)
{
  int indice;

  while (true)
  {
    indice = producir();
    estanco->ponerIngrediente(indice);
    estanco->esperaIngrediente();
  }
}

//----------------------------------------------------------------------

int main()
{
  cout << "--------------------------------------------------------" << endl
       << "Problema de los fumadores." << endl
       << "--------------------------------------------------------" << endl
       << flush;

  auto monitor = Create<Estanco>();
  thread hebra_fumadores[num_fumadores];
  for (int i = 0; i < num_fumadores; i++)
    hebra_fumadores[i] = thread(funcion_hebra_fumador, i, monitor);

  thread estanquero(funcion_hebra_estanquero, monitor);

  for (int i = 0; i < num_fumadores; i++)
    hebra_fumadores[i].join();
  estanquero.join();

  return 0;
}
