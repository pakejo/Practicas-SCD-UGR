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

const int num_clientes = 10;

template <int min, int max>
int aleatorio()
{
  static default_random_engine generador((random_device())());
  static uniform_int_distribution<int> distribucion_uniforme(min, max);
  return distribucion_uniforme(generador);
}

void EsperarFueraBarberia(int n_cliente)
{
  chrono::milliseconds duracion_espera(aleatorio<20,200>());

  cout <<"Cliente " <<n_cliente <<" espera fuera de la barberia" <<endl <<flush;

  this_thread::sleep_for(duracion_espera);

  cout <<"Cliente " <<n_cliente <<" termina de esperar y entra en la barberia" <<endl <<flush;
}

void CortarPeloCliente()
{
  chrono::milliseconds duracion_corte(aleatorio<20,200>());

  cout <<"Cortando pelo a cliente " <<endl <<flush;

  this_thread::sleep_for(duracion_corte);

  cout <<"Cliente termina de cortarse el pelo" <<endl <<flush;
}

class Barberia : public HoareMonitor
{
  private:
    CondVar
      silla,        //Variable condicion para la silla
      sala_espera,  //Cola de clientes en la sala de espera
      barbero;      //Variable condicion para el barbero

  public:
    Barberia();
    void CortarPelo(int n_cliente); //El cliente pide para cortarse el pelo
    void SiguienteCliente();        //El barbero llama al siguiente cliente (si hay)
    void FinCliente();              //Cliente termina y se va
};

Barberia::Barberia()
{
  silla         = newCondVar();
  sala_espera   = newCondVar();
  barbero       = newCondVar();
}

void Barberia::CortarPelo(int n_cliente)
{
  cout <<"Cliente " <<n_cliente <<" llega a la barberia" <<endl;

  if(!silla.empty())
  {
    cout <<"Barbero cortando el pelo, el cliente para a sala de espera" <<endl;
    sala_espera.wait();
  }

  cout <<"Barbero llama a " <<n_cliente <<" y se siente en la silla" <<endl;

  //Llama al barbero
  if(!barbero.empty())
    barbero.signal();

  //Espera en la silla a que llegue el barbero
  silla.wait();
}

void Barberia::SiguienteCliente()
{
  cout <<"Barbero llama al siguiente cliente" <<endl;

  if(silla.empty() && !sala_espera.empty())
    sala_espera.signal();

  if(silla.empty() && sala_espera.empty())
  {
    cout <<"No hay clientes en la sala de espera, barbero esperando..." <<endl;
    barbero.wait();
  }
}

void Barberia::FinCliente()
{
  cout <<"Barbero termina de cortar el pelo. Sale el cliente" <<endl;
  silla.signal();
}

void funcion_hebra_cliente(MRef<Barberia> barberia, int n_cliente)
{
  while (true)
  {
    barberia->CortarPelo(n_cliente);
    EsperarFueraBarberia(n_cliente);
  }
}

void funcion_hebra_barbero(MRef<Barberia> barberia)
{
  while (true)
  {
    barberia->SiguienteCliente();
    CortarPeloCliente();
    barberia->FinCliente();
  }
}

int main()
{
  cout << "--------------------------------------------------------" << endl
       << "Problema de la barbería." << endl
       << "--------------------------------------------------------" << endl
       << flush;

  MRef<Barberia> monitor = Create<Barberia>();
  thread hebra_clientes[num_clientes];

  thread barbero(funcion_hebra_barbero, monitor);

  for (int i = 0; i < num_clientes; i++)
    hebra_clientes[i] = thread(funcion_hebra_cliente, monitor, i);

  for (int i = 0; i < num_clientes; i++)
    hebra_clientes[i].join();
  barbero.join();

  return 0;
}