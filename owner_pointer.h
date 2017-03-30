#pragma once

#include "stdafx.h"
#include "util.h"

template <typename T>
class WeakPointer;

template <typename T>
class OwnedObject;

template <typename T>
class OwnerPointer {
  public:

  template <typename U>
  OwnerPointer(OwnerPointer<U>&& o) : elem(std::move(o.elem)) {
  }

  OwnerPointer() {}
  OwnerPointer(std::nullptr_t) {}


  OwnerPointer<T>& operator = (OwnerPointer<T>&& o) {
    elem = std::move(o.elem);
    return *this;
  }

  void clear() {
    elem.reset();
  }

  T* operator -> () const {
    return elem.get();
  }

  T& operator * () const {
    return *elem;
  }

  WeakPointer<T> get() const;

  operator bool() const {
    return !!elem;
  }

  bool operator !() const {
    return !elem;
  }

/*  weak_ptr<T> getWeakPointer() const {
    return weak_ptr<T>(elem);
  }*/

  SERIALIZE_ALL(elem)

  private:
  template <typename>
  friend class OwnerPointer;

  template <typename U, typename... Args>
  friend OwnerPointer<U> makeOwner(Args&&... a);

  explicit OwnerPointer(shared_ptr<T> t) : elem(t) {}

  shared_ptr<T> SERIAL(elem);
};

template <typename T>
class WeakPointer {
  public:

  template <typename U>
  WeakPointer(const WeakPointer<U>& o) : elem(o.elem) {
  }

  WeakPointer(T* t) : elem(std::dynamic_pointer_cast<T>(t->shared_from_this())) {}

  WeakPointer() {}
  WeakPointer(std::nullptr_t) {}

  /*WeakPointer<T>& operator = (WeakPointer<T>&& o) {
    elem = std::move(o.elem);
    return *this;
  }*/

  void clear() {
    elem.reset();
  }

  T* operator -> () const {
    return elem.lock().get();
  }

  T& operator * () const {
    return *elem.lock().get();
  }

  T* get() const {
    return elem.lock().get();
  }

  operator bool() const {
    return !!elem.lock();
  }

  bool operator !() const {
    return !elem.lock();
  }

  template <typename U>
  bool operator == (const WeakPointer<U>& o) const {
    return get() == o.get();
  }

  template <typename U>
  bool operator != (const WeakPointer<U>& o) const {
    return !(*this == o);
  }

  bool operator == (std::nullptr_t) const {
    return !elem.lock();
  }

  bool operator != (std::nullptr_t) const {
    return !!elem.lock();
  }

  SERIALIZE_ALL(elem)

  private:

  template <typename>
  friend class OwnedObject;
  template <typename>
  friend class OwnerPointer;
  template <typename>
  friend class WeakPointer;
  WeakPointer(const shared_ptr<T>& e) : elem(e) {}

  weak_ptr<T> SERIAL(elem);
};

template <typename T>
class OwnedObject : public std::enable_shared_from_this<T> {
  public:
  WeakPointer<T> getThis() {
    return WeakPointer<T>(this->shared_from_this());
  }

  WeakPointer<const T> getThis() const {
    return WeakPointer<const T>(this->shared_from_this());
  }

  template <typename U>
  WeakPointer<U> getThisAs() {
    return WeakPointer<U>(std::dynamic_pointer_cast<U>(this->shared_from_this()));
  }

  template <typename U>
  WeakPointer<const U> getThisAs() const {
    return WeakPointer<const U>(std::dynamic_pointer_cast<const U>(this->shared_from_this()));
  }
};

template <typename T>
WeakPointer<T> OwnerPointer<T>::get() const {
  return WeakPointer<T>(elem);
}

template <typename T, typename... Args>
OwnerPointer<T> makeOwner(Args&&... args) {
  return OwnerPointer<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

template<class T>
vector<WeakPointer<T>> getWeakPointers(const vector<OwnerPointer<T>>& v) {
  vector<WeakPointer<T>> ret;
  ret.reserve(v.size());
  for (auto& el : v)
    ret.push_back(el.get());
  return ret;
}
