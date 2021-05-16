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
      .def(py::init<std::string, std::string, int, int, int, int,
	   std::string, std::string, std::string, std::string, int>(),
	   py::arg("vocab_file"),
	   py::arg("codes_file"),
	   py::arg("pad")=0,
	   py::arg("start")=1,
	   py::arg("end")=2,
	   py::arg("unk")=3,
	   py::arg("pad_str")="<PAD>",
	   py::arg("start_str")="<GO>",
	   py::arg("end_str")="<EOS>",
	   py::arg("unk_str")="<UNK>",
	   py::arg("offset")=4
	   
	   )
      .def("lookup", &BPEVocab::lookup)
      .def_property_readonly("pad_id", &BPEVocab::pad_id)
      .def_property_readonly("start_id", &BPEVocab::start_id)
      .def_property_readonly("end_id", &BPEVocab::end_id)
      .def_property_readonly("unk_id", &BPEVocab::unk_id)
      .def_property_readonly("pad_str", &BPEVocab::pad_str)
      .def_property_readonly("start_str", &BPEVocab::start_str)
      .def_property_readonly("end_str", &BPEVocab::end_str)
      .def_property_readonly("unk_str", &BPEVocab::unk_str)
      .def("apply", &BPEVocab::apply,
	   py::arg("tokens")
	   )
      ;

    py::class_<VocabVectorizer>(m, "VocabVectorizer")
      .def(py::init<BPEVocab*, TokenList_T, TokenList_T>(),
	   py::arg("vocab"),
	   py::arg("emit_begin_tok"),
	   py::arg("emit_end_tok"))
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
