#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

#include "params/MM.hh"
#include "python/pybind11/core.hh"
#include "sim/init.hh"
#include "sim/sim_object.hh"

#include "mem/cache/tags/mm.hh"

namespace py = pybind11;

static void
module_init(py::module &m_internal)
{
    py::module m = m_internal.def_submodule("param_MM");
    py::class_<MMParams, BaseTagsParams, std::unique_ptr<MMParams, py::nodelete>>(m, "MMParams")
        .def(py::init<>())
        .def("create", &MMParams::create)
        ;

    py::class_<MM, BaseTags, std::unique_ptr<MM, py::nodelete>>(m, "MM")
        ;

}

static EmbeddedPyBind embed_obj("MM", module_init, "BaseTags");
