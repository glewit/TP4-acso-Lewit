/* 
Entrego esta version de thread-pool no funcionando correctamente. Para entender bien el funcionamiento de thread-pool,
decidi intentar implementarlo utilizando el dispatcher como un worker mas. Esa implementacion logre hacerla funcionar
(puede verse en thread-pool-nodispatcher.cc y cambiando el Makefile se puede utilizar), pero al intentar partir de esa
implementacion para la pedida por la consigna, no pude hacerlo funcionar correctamente. No logre encontrar bien el error;
no supe si esta entre wait y los destructores o si mi error es mas conceptual del dispatcher y workers. A su vez, por una 
rotura de ligamento me atrase con la entrega y, como tuve que hacerme estudios, no pude conectarme a las clases de consulta.
*/

#include "thread-pool.h"
#include <iostream>
  
using namespace std;

ThreadPool::ThreadPool(size_t numThreads)
  : wts(numThreads), semaphore(0), stop(false) {
  dt = thread(&ThreadPool::dispatcher, this);

  for (size_t i = 0; i < numThreads; ++i) {
    wts[i].available = true;
    wts[i].task = nullptr;
    wts[i].thread = thread(&ThreadPool::worker, this, i);
  }
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
  mutexTareas.lock();
  tareas.push(thunk);
  mutexTareas.unlock();
  semaphore.signal();
}

void ThreadPool::wait() {
  mutexTareas.lock();
  stop = true;
  mutexTareas.unlock();

  for (size_t i = 0; i < wts.size() + 1; ++i) {
    semaphore.signal();
  }

  dt.join();
}


ThreadPool::~ThreadPool() {
  if (dt.joinable()) {
    wait();
  }

  for (auto& wt : wts) {
    if (wt.thread.joinable()) {
      wt.workerSemaphore.signal();
      wt.thread.join();
    }
  }
}

void ThreadPool::dispatcher() {
  while (true) {
    semaphore.wait();
    mutexTareas.lock();

    if (stop && tareas.empty()) {
      mutexTareas.unlock();
      break;
    }

    if (!tareas.empty()) {
      for (auto& wt : wts) {
        if (wt.available) {
          auto task = tareas.front();
          tareas.pop();
          wt.available = false;
          mutexTareas.unlock();

          wt.mutexTareaWorker.lock();
          wt.task = task;
          wt.mutexTareaWorker.unlock();

          wt.workerSemaphore.signal();
          break;
        }
      }
    } else {
      mutexTareas.unlock();
    }
  }
}

void ThreadPool::worker(size_t workerID) {
  Worker& wt = wts[workerID];
  while (true) {
    wt.workerSemaphore.wait();
    wt.mutexTareaWorker.lock();

    if (stop && wt.task == nullptr) {
      wt.mutexTareaWorker.unlock();
      break;
    }

    if (wt.task) {
      wt.task();
      wt.task = nullptr;
      wt.available = true;
    }

    wt.mutexTareaWorker.unlock();
    
  }
}