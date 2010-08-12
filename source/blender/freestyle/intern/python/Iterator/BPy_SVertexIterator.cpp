#include "BPy_SVertexIterator.h"

#include "../BPy_Convert.h"
#include "../Interface0D/BPy_SVertex.h"
#include "../Interface1D/BPy_FEdge.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////

//------------------------INSTANCE METHODS ----------------------------------

static char SVertexIterator___doc__[] =
"Class hierarchy: :class:`Iterator` > :class:`SVertexIterator`\n"
"\n"
"Class representing an iterator over :class:`SVertex` of a\n"
":class:`ViewEdge`.  An instance of an SVertexIterator can be obtained\n"
"from a ViewEdge by calling verticesBegin() or verticesEnd().\n"
"\n"
".. method:: __init__()\n"
"\n"
"   Default constructor.\n"
"\n"
".. method:: __init__(it)\n"
"\n"
"   Copy constructor.\n"
"\n"
"   :arg it: An SVertexIterator object.\n"
"   :type it: :class:`SVertexIterator`\n"
"\n"
".. method:: __init__(v, begin, prev, next, t)\n"
"\n"
"   Builds an SVertexIterator that starts iteration from an SVertex\n"
"   object v.\n"
"\n"
"   :arg v: The SVertex from which the iterator starts iteration.\n"
"   :type v: :class:`SVertex`\n"
"   :arg begin: The first vertex of a view edge.\n"
"   :type begin: :class:`SVertex`\n"
"   :arg prev: The previous FEdge coming to v.\n"
"   :type prev: :class:`FEdge`\n"
"   :arg next: The next FEdge going out from v.\n"
"   :type next: :class:`FEdge`\n"
"   :arg t: The curvilinear abscissa at v.\n"
"   :type t: float\n";

static int SVertexIterator___init__(BPy_SVertexIterator *self, PyObject *args )
{	
	PyObject *obj1 = 0, *obj2 = 0, *obj3 = 0, *obj4 = 0;
	float f = 0;

	if (! PyArg_ParseTuple(args, "|OOOOf", &obj1, &obj2, &obj3, &obj4, f) )
	    return -1;

	if( !obj1 ){
		self->sv_it = new ViewEdgeInternal::SVertexIterator();
		
	} else if( BPy_SVertexIterator_Check(obj1) ) {
		self->sv_it = new ViewEdgeInternal::SVertexIterator(*( ((BPy_SVertexIterator *) obj1)->sv_it ));
	
	} else if(  obj1 && BPy_SVertex_Check(obj1) &&
	 			obj2 && BPy_SVertex_Check(obj2) &&
				obj3 && BPy_FEdge_Check(obj3) &&
				obj4 && BPy_FEdge_Check(obj4) ) {

		self->sv_it = new ViewEdgeInternal::SVertexIterator(
							((BPy_SVertex *) obj1)->sv,
							((BPy_SVertex *) obj2)->sv,
							((BPy_FEdge *) obj3)->fe,
							((BPy_FEdge *) obj4)->fe,
							f );
			
	} else {
		PyErr_SetString(PyExc_TypeError, "invalid argument(s)");
		return -1;
	}

	self->py_it.it = self->sv_it;

	return 0;
}

static char SVertexIterator_t___doc__[] =
".. method:: t()\n"
"\n"
"   Returns the curvilinear abscissa.\n"
"\n"
"   :return: The curvilinear abscissa.\n"
"   :rtype: float\n";

static PyObject * SVertexIterator_t( BPy_SVertexIterator *self ) {
	return PyFloat_FromDouble( self->sv_it->t() );
}

static char SVertexIterator_u___doc__[] =
".. method:: u()\n"
"\n"
"   Returns the point parameter (0<=u<=1).\n"
"\n"
"   :return: The point parameter.\n"
"   :rtype: float\n";

static PyObject * SVertexIterator_u( BPy_SVertexIterator *self ) {
	return PyFloat_FromDouble( self->sv_it->u() );
}

static char SVertexIterator_getObject___doc__[] =
".. method:: getObject()\n"
"\n"
"   Returns the pointed SVertex.\n"
"\n"
"   :return: the pointed SVertex.\n"
"   :rtype: :class:`SVertex`\n";

static PyObject * SVertexIterator_getObject( BPy_SVertexIterator *self) {
	SVertex *sv = self->sv_it->operator->();
	
	if( sv )
		return BPy_SVertex_from_SVertex( *sv );
		
	Py_RETURN_NONE;	
}

/*----------------------SVertexIterator instance definitions ----------------------------*/
static PyMethodDef BPy_SVertexIterator_methods[] = {
	{"t", ( PyCFunction ) SVertexIterator_t, METH_NOARGS, SVertexIterator_t___doc__},
	{"u", ( PyCFunction ) SVertexIterator_u, METH_NOARGS, SVertexIterator_u___doc__},
	{"getObject", ( PyCFunction ) SVertexIterator_getObject, METH_NOARGS, SVertexIterator_getObject___doc__},
	{NULL, NULL, 0, NULL}
};

/*-----------------------BPy_SVertexIterator type definition ------------------------------*/

PyTypeObject SVertexIterator_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"SVertexIterator",              /* tp_name */
	sizeof(BPy_SVertexIterator),    /* tp_basicsize */
	0,                              /* tp_itemsize */
	0,                              /* tp_dealloc */
	0,                              /* tp_print */
	0,                              /* tp_getattr */
	0,                              /* tp_setattr */
	0,                              /* tp_reserved */
	0,                              /* tp_repr */
	0,                              /* tp_as_number */
	0,                              /* tp_as_sequence */
	0,                              /* tp_as_mapping */
	0,                              /* tp_hash  */
	0,                              /* tp_call */
	0,                              /* tp_str */
	0,                              /* tp_getattro */
	0,                              /* tp_setattro */
	0,                              /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	SVertexIterator___doc__,        /* tp_doc */
	0,                              /* tp_traverse */
	0,                              /* tp_clear */
	0,                              /* tp_richcompare */
	0,                              /* tp_weaklistoffset */
	0,                              /* tp_iter */
	0,                              /* tp_iternext */
	BPy_SVertexIterator_methods,    /* tp_methods */
	0,                              /* tp_members */
	0,                              /* tp_getset */
	&Iterator_Type,                 /* tp_base */
	0,                              /* tp_dict */
	0,                              /* tp_descr_get */
	0,                              /* tp_descr_set */
	0,                              /* tp_dictoffset */
	(initproc)SVertexIterator___init__, /* tp_init */
	0,                              /* tp_alloc */
	0,                              /* tp_new */
};

///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
