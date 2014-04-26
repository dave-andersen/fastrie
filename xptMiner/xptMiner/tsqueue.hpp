/*
 * Threadsafe blocking work queue implementation
 * Defined to be analogous to STL deque;  please follow STL
 * naming conventions for methods.
 */

#ifndef _TSQUEUE_H_
#define _TSQUEUE_H_

#include "global.h" /* Provides CRITICAL_SECTION */

#include <deque>

template<class T, int maxSize>
class ts_queue {
private:
  std::deque<T> _q;
  CRITICAL_SECTION _m;
  CONDITION_VARIABLE _cv;
  CONDITION_VARIABLE _cv_full;

public:
  ts_queue() {
    InitializeCriticalSection(&_m);
    InitializeConditionVariable(&_cv);
    InitializeConditionVariable(&_cv_full);
  }

  /* Blocks iff queue size >= maxSize */
  void push_back(T item) {
    EnterCriticalSection(&_m);
    while (_q.size() >= maxSize) {
      SleepConditionVariableCS(&_cv_full, &_m, 2000);
    }
    _q.push_back(item);
    LeaveCriticalSection(&_m);
    WakeConditionVariable(&_cv);
  }

  void push_front(T item) {
    EnterCriticalSection(&_m);
    while (_q.size() >= maxSize) {
      SleepConditionVariableCS(&_cv_full, &_m, 2000);
    }
    _q.push_front(item);
    LeaveCriticalSection(&_m);
    WakeConditionVariable(&_cv);
  }


  /* Blocks until an item is available to pop */
  T pop_front() {
    EnterCriticalSection(&_m);
    while (_q.empty()) {
      SleepConditionVariableCS(&_cv, &_m, 2000);
    }
    /* Pop and notify sleeping inserters */
    auto r = _q.front();
    _q.pop_front();
    LeaveCriticalSection(&_m);
    WakeConditionVariable(&_cv_full);
    return r;
  }
  
  /* Nonblocking - clears queue, returns number of items removed */
  typename std::deque<T>::size_type clear() {
    EnterCriticalSection(&_m);
    auto s = _q.size();
    _q.clear();
    //while (!_q.empty()) {
    //_q.pop();
    //}
    LeaveCriticalSection(&_m);
    WakeAllConditionVariable(&_cv_full);
    return s;
  }

  int size() {
    EnterCriticalSection(&_m);
    auto s = _q.size();
    LeaveCriticalSection(&_m);
    return s;
  }

};

#endif /* _TSQUEUE_H_ */
