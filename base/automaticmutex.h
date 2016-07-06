#ifndef AUTOMATICMUTEX_H
#define AUTOMATICMUTEX_H

namespace base
{
  template<typename T> class AutoMutex
  {
  private:
    T* _mtx;
    bool _locked;

  public:
    AutoMutex() :
      _mtx(NULL),
      _locked(false)
    {}

    AutoMutex(T& mutex) :
      _mtx(&mutex),
      _locked(false)
    {}

    AutoMutex(T* mutex) :
      _mtx(mutex),
      _locked(false)
    {}

    inline AutoMutex(void* mutex) :
      AutoMutex((T*) mutex)
    {}

    ~AutoMutex()
    {
      if(_locked)
      {
        _mtx->unlock();
        _locked = false;
      }
    }

    AutoMutex& lock()
    {
      if(_locked) return (*this);
      _mtx->lock();
      _locked = true;
      return (*this);
    }

    AutoMutex& unlock()
    {
      if(_locked)
      {
        _mtx->unlock();
      }
      return (*this);
    }
  };
}

#endif // AUTOMATICMUTEX_H
