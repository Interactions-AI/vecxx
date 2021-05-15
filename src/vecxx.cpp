#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "vecxx/vecxx.h"

namespace py = pybind11;

PYBIND11_MODULE(vecxx, m) {
    #ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
    #endif
    m.doc() = "pybind11 vecxx plugin";
    py::class_<BPEVocab>(m, "BPEVocab")
      .def(py::init<const std::string&, const std::string&, int, int>(),
	   py::arg("vocab_file"),
	   py::arg("codes_file"),
	   py::arg("offset")=4,
	   py::arg("unk")=3)
      .def("lookup", &BPEVocab::lookup)
      .def("apply", &BPEVocab::apply,
	   py::arg("tokens")
	   )
      ;

    py::class_<VocabVectorizer>(m, "VocabVectorizer")
      .def(py::init<BPEVocab*, std::string, std::string>(),
	   py::arg("vocab"),
	   py::arg("emit_begin_tok")="",
	   py::arg("emit_end_tok")="")
      .def("piece_to_id", &VocabVectorizer::piece_to_id)
      .def("convert_to_pieces", &VocabVectorizer::convert_to_pieces,
	   py::arg("tokens")
	   )
      .def("convert_to_ids", &VocabVectorizer::convert_to_ids,
	   py::arg("tokens"),
	   py::arg("max_len")=0
	   )
      ;


    
  };
