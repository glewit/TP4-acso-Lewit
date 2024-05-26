#include "thread-pool-nodispatcher.h"
#include <iostream>

using namespace std;

ThreadPool::ThreadPool(size_t numThreads) 
  : wts(numThreads), semaphore(0), stop(false) {
  dt = thread(&ThreadPool::dispatcher, this);

  for (size_t i = 0; i < numThreads; ++i) {
    wts[i] = thread(&ThreadPool::worker, this, i);
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
    if (wt.joinable()) {
      wt.join();
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
      auto task = tareas.front();
      tareas.pop();
      mutexTareas.unlock();
      task();
    } else {
      mutexTareas.unlock();
    }

  }
}

void ThreadPool::worker(size_t workerID) {
  while (true) {
    semaphore.wait();
    mutexTareas.lock();

    if (stop && tareas.empty()) {
      mutexTareas.unlock();
      break;
    }

    if (!tareas.empty()) {
      auto task = tareas.front();
      tareas.pop();
      mutexTareas.unlock();
      task();
    } else {
      mutexTareas.unlock();
    }
    
  }
}
