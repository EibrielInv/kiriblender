/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.35
 * 
 * This file is not intended to be easily readable and contains a number of 
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG 
 * interface file instead. 
 * ----------------------------------------------------------------------------- */

#ifndef SWIG_Freestyle_WRAP_H_
#define SWIG_Freestyle_WRAP_H_

#include <map>
#include <string>


class SwigDirector_ViewEdgeViewEdgeIterator : public ViewEdgeInternal::ViewEdgeIterator, public Swig::Director {

public:
    SwigDirector_ViewEdgeViewEdgeIterator(PyObject *self, ViewEdge *begin = 0, bool orientation = true);
    SwigDirector_ViewEdgeViewEdgeIterator(PyObject *self, ViewEdgeInternal::ViewEdgeIterator const &it);
    virtual ~SwigDirector_ViewEdgeViewEdgeIterator();
    virtual std::string getExactTypeName() const;
    virtual ViewEdge *operator *();
    virtual ViewEdge *operator ->();
    virtual ViewEdgeInternal::ViewEdgeIterator &operator ++();
    virtual ViewEdgeInternal::ViewEdgeIterator operator ++(int arg0);
    virtual void increment();
    virtual ViewEdgeInternal::ViewEdgeIterator &operator --();
    virtual ViewEdgeInternal::ViewEdgeIterator operator --(int arg0);
    virtual void decrement();
    virtual bool isBegin() const;
    virtual bool isEnd() const;
    virtual bool operator ==(ViewEdgeInternal::ViewEdgeIterator &it) const;
    virtual bool operator !=(ViewEdgeInternal::ViewEdgeIterator &it) const;


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class ViewEdgeViewEdgeIterator doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[9];
#endif

};


class SwigDirector_UnaryFunction0DVoid : public UnaryFunction0D< void >, public Swig::Director {

public:
    SwigDirector_UnaryFunction0DVoid(PyObject *self);
    virtual ~SwigDirector_UnaryFunction0DVoid();
    virtual std::string getName() const;
    virtual void operator ()(Interface0DIterator &iter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction0DVoid doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryFunction0DUnsigned : public UnaryFunction0D< unsigned int >, public Swig::Director {

public:
    SwigDirector_UnaryFunction0DUnsigned(PyObject *self);
    virtual ~SwigDirector_UnaryFunction0DUnsigned();
    virtual std::string getName() const;
    virtual unsigned int operator ()(Interface0DIterator &iter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction0DUnsigned doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryFunction0DFloat : public UnaryFunction0D< float >, public Swig::Director {

public:
    SwigDirector_UnaryFunction0DFloat(PyObject *self);
    virtual ~SwigDirector_UnaryFunction0DFloat();
    virtual std::string getName() const;
    virtual float operator ()(Interface0DIterator &iter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction0DFloat doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryFunction0DDouble : public UnaryFunction0D< double >, public Swig::Director {

public:
    SwigDirector_UnaryFunction0DDouble(PyObject *self);
    virtual ~SwigDirector_UnaryFunction0DDouble();
    virtual std::string getName() const;
    virtual double operator ()(Interface0DIterator &iter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction0DDouble doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryFunction0DVec2f : public UnaryFunction0D< Geometry::Vec2f >, public Swig::Director {

public:
    SwigDirector_UnaryFunction0DVec2f(PyObject *self);
    virtual ~SwigDirector_UnaryFunction0DVec2f();
    virtual std::string getName() const;
    virtual VecMat::Vec2< float > operator ()(Interface0DIterator &iter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction0DVec2f doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryFunction0DVec3f : public UnaryFunction0D< Geometry::Vec3f >, public Swig::Director {

public:
    SwigDirector_UnaryFunction0DVec3f(PyObject *self);
    virtual ~SwigDirector_UnaryFunction0DVec3f();
    virtual std::string getName() const;
    virtual VecMat::Vec3< float > operator ()(Interface0DIterator &iter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction0DVec3f doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryFunction0DId : public UnaryFunction0D< Id >, public Swig::Director {

public:
    SwigDirector_UnaryFunction0DId(PyObject *self);
    virtual ~SwigDirector_UnaryFunction0DId();
    virtual std::string getName() const;
    virtual Id operator ()(Interface0DIterator &iter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction0DId doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryFunction1DVoid : public UnaryFunction1D< void >, public Swig::Director {

public:
    SwigDirector_UnaryFunction1DVoid(PyObject *self);
    SwigDirector_UnaryFunction1DVoid(PyObject *self, IntegrationType iType);
    virtual ~SwigDirector_UnaryFunction1DVoid();
    virtual std::string getName() const;
    virtual void operator ()(Interface1D &inter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction1DVoid doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryFunction1DUnsigned : public UnaryFunction1D< unsigned int >, public Swig::Director {

public:
    SwigDirector_UnaryFunction1DUnsigned(PyObject *self);
    SwigDirector_UnaryFunction1DUnsigned(PyObject *self, IntegrationType iType);
    virtual ~SwigDirector_UnaryFunction1DUnsigned();
    virtual std::string getName() const;
    virtual unsigned int operator ()(Interface1D &inter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction1DUnsigned doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryFunction1DFloat : public UnaryFunction1D< float >, public Swig::Director {

public:
    SwigDirector_UnaryFunction1DFloat(PyObject *self);
    SwigDirector_UnaryFunction1DFloat(PyObject *self, IntegrationType iType);
    virtual ~SwigDirector_UnaryFunction1DFloat();
    virtual std::string getName() const;
    virtual float operator ()(Interface1D &inter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction1DFloat doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryFunction1DDouble : public UnaryFunction1D< double >, public Swig::Director {

public:
    SwigDirector_UnaryFunction1DDouble(PyObject *self);
    SwigDirector_UnaryFunction1DDouble(PyObject *self, IntegrationType iType);
    virtual ~SwigDirector_UnaryFunction1DDouble();
    virtual std::string getName() const;
    virtual double operator ()(Interface1D &inter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction1DDouble doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryFunction1DVec2f : public UnaryFunction1D< Geometry::Vec2f >, public Swig::Director {

public:
    SwigDirector_UnaryFunction1DVec2f(PyObject *self);
    SwigDirector_UnaryFunction1DVec2f(PyObject *self, IntegrationType iType);
    virtual ~SwigDirector_UnaryFunction1DVec2f();
    virtual std::string getName() const;
    virtual VecMat::Vec2< float > operator ()(Interface1D &inter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction1DVec2f doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryFunction1DVec3f : public UnaryFunction1D< Geometry::Vec3f >, public Swig::Director {

public:
    SwigDirector_UnaryFunction1DVec3f(PyObject *self);
    SwigDirector_UnaryFunction1DVec3f(PyObject *self, IntegrationType iType);
    virtual ~SwigDirector_UnaryFunction1DVec3f();
    virtual std::string getName() const;
    virtual VecMat::Vec3< float > operator ()(Interface1D &inter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryFunction1DVec3f doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_ChainingIterator : public ChainingIterator, public Swig::Director {

public:
    SwigDirector_ChainingIterator(PyObject *self, bool iRestrictToSelection = true, bool iRestrictToUnvisited = true, ViewEdge *begin = 0, bool orientation = true);
    SwigDirector_ChainingIterator(PyObject *self, ChainingIterator const &brother);
    virtual ~SwigDirector_ChainingIterator();
    virtual std::string getExactTypeName() const;
    virtual ViewEdge *operator *();
    virtual ViewEdge *operator ->();
    virtual ViewEdgeInternal::ViewEdgeIterator &operator ++();
    virtual ViewEdgeInternal::ViewEdgeIterator operator ++(int arg0);
    virtual void increment();
    virtual ViewEdgeInternal::ViewEdgeIterator &operator --();
    virtual ViewEdgeInternal::ViewEdgeIterator operator --(int arg0);
    virtual void decrement();
    virtual bool isBegin() const;
    virtual bool isEnd() const;
    virtual bool operator ==(ViewEdgeInternal::ViewEdgeIterator &it) const;
    virtual bool operator !=(ViewEdgeInternal::ViewEdgeIterator &it) const;
    virtual void init();
    virtual ViewEdge *traverse(AdjacencyIterator const &it);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class ChainingIterator doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[11];
#endif

};


class SwigDirector_ChainSilhouetteIterator : public ChainSilhouetteIterator, public Swig::Director {

public:
    SwigDirector_ChainSilhouetteIterator(PyObject *self, bool iRestrictToSelection = true, ViewEdge *begin = NULL, bool orientation = true);
    SwigDirector_ChainSilhouetteIterator(PyObject *self, ChainSilhouetteIterator const &brother);
    virtual ~SwigDirector_ChainSilhouetteIterator();
    virtual std::string getExactTypeName() const;
    virtual ViewEdge *operator *();
    virtual ViewEdge *operator ->();
    virtual ViewEdgeInternal::ViewEdgeIterator &operator ++();
    virtual ViewEdgeInternal::ViewEdgeIterator operator ++(int arg0);
    virtual void increment();
    virtual ViewEdgeInternal::ViewEdgeIterator &operator --();
    virtual ViewEdgeInternal::ViewEdgeIterator operator --(int arg0);
    virtual void decrement();
    virtual bool isBegin() const;
    virtual bool isEnd() const;
    virtual bool operator ==(ViewEdgeInternal::ViewEdgeIterator &it) const;
    virtual bool operator !=(ViewEdgeInternal::ViewEdgeIterator &it) const;
    virtual void init();
    virtual ViewEdge *traverse(AdjacencyIterator const &it);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class ChainSilhouetteIterator doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[11];
#endif

};


class SwigDirector_ChainPredicateIterator : public ChainPredicateIterator, public Swig::Director {

public:
    SwigDirector_ChainPredicateIterator(PyObject *self, bool iRestrictToSelection = true, bool iRestrictToUnvisited = true, ViewEdge *begin = NULL, bool orientation = true);
    SwigDirector_ChainPredicateIterator(PyObject *self, UnaryPredicate1D &upred, BinaryPredicate1D &bpred, bool iRestrictToSelection = true, bool iRestrictToUnvisited = true, ViewEdge *begin = NULL, bool orientation = true);
    SwigDirector_ChainPredicateIterator(PyObject *self, ChainPredicateIterator const &brother);
    virtual ~SwigDirector_ChainPredicateIterator();
    virtual std::string getExactTypeName() const;
    virtual ViewEdge *operator *();
    virtual ViewEdge *operator ->();
    virtual ViewEdgeInternal::ViewEdgeIterator &operator ++();
    virtual ViewEdgeInternal::ViewEdgeIterator operator ++(int arg0);
    virtual void increment();
    virtual ViewEdgeInternal::ViewEdgeIterator &operator --();
    virtual ViewEdgeInternal::ViewEdgeIterator operator --(int arg0);
    virtual void decrement();
    virtual bool isBegin() const;
    virtual bool isEnd() const;
    virtual bool operator ==(ViewEdgeInternal::ViewEdgeIterator &it) const;
    virtual bool operator !=(ViewEdgeInternal::ViewEdgeIterator &it) const;
    virtual void init();
    virtual ViewEdge *traverse(AdjacencyIterator const &it);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class ChainPredicateIterator doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[11];
#endif

};


class SwigDirector_UnaryPredicate0D : public UnaryPredicate0D, public Swig::Director {

public:
    SwigDirector_UnaryPredicate0D(PyObject *self);
    virtual ~SwigDirector_UnaryPredicate0D();
    virtual std::string getName() const;
    virtual bool operator ()(Interface0DIterator &it);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryPredicate0D doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_UnaryPredicate1D : public UnaryPredicate1D, public Swig::Director {

public:
    SwigDirector_UnaryPredicate1D(PyObject *self);
    virtual ~SwigDirector_UnaryPredicate1D();
    virtual std::string getName() const;
    virtual bool operator ()(Interface1D &inter);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class UnaryPredicate1D doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_BinaryPredicate1D : public BinaryPredicate1D, public Swig::Director {

public:
    SwigDirector_BinaryPredicate1D(PyObject *self);
    virtual ~SwigDirector_BinaryPredicate1D();
    virtual std::string getName() const;
    virtual bool operator ()(Interface1D &inter1, Interface1D &inter2);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class BinaryPredicate1D doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


class SwigDirector_StrokeShader : public StrokeShader, public Swig::Director {

public:
    SwigDirector_StrokeShader(PyObject *self);
    virtual ~SwigDirector_StrokeShader();
    virtual std::string getName() const;
    virtual void shade(Stroke &ioStroke) const;


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* name) const {
      std::map<std::string, bool>::const_iterator iv = inner.find(name);
      return (iv != inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* name, bool val) const
    { inner[name] = val;}

private:
    mutable std::map<std::string, bool> inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::PyObject_var name = PyString_FromString(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (method == NULL) {
          std::string msg = "Method in class StrokeShader doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::PyObject_var vtable[2];
#endif

};


#endif
