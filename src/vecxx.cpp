#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "vecxx/vecxx.h"
#define STRINGIFY(x) #x
namespace py = pybind11;

PYBIND11_MODULE(vecxx, m) {

    #ifdef VERSION_INFO
        m.attr("__version__") = STRINGIFY(VERSION_INFO);
    #else
        m.attr("__version__") = "dev";
    #endif
    m.doc() = "pybind11 vecxx plugin";
    py::class_<Vocab>(m, "Vocab")
      .def("lookup", &Vocab::lookup)
      .def("apply", &Vocab::apply)
      ;
    py::class_<BPEVocab, Vocab>(m, "BPEVocab")
      .def(py::init<std::string, std::string, Index_T, Index_T, Index_T, Index_T,
	   std::string, std::string, std::string, std::string, const TokenList_T&>(),
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
	   py::arg("extra_tokens")=TokenList_T()
	   
	   )
      .def("lookup", &BPEVocab::lookup)
      .def("compile_vocab", &BPEVocab::compile_vocab)
      .def_property_readonly("pad_id", &BPEVocab::pad_id)
      .def_property_readonly("start_id", &BPEVocab::start_id)
      .def_property_readonly("end_id", &BPEVocab::end_id)
      .def_property_readonly("unk_id", &BPEVocab::unk_id)
      .def_property_readonly("pad_str", &BPEVocab::pad_str)
      .def_property_readonly("start_str", &BPEVocab::start_str)
      .def_property_readonly("end_str", &BPEVocab::end_str)
      .def_property_readonly("unk_str", &BPEVocab::unk_str)
      .def_readonly("special_tokens", &BPEVocab::special_tokens)
      .def_readonly("vocab", &BPEVocab::vocab)
      .def("apply", &BPEVocab::apply)
      ;
      
    py::class_<WordVocab, Vocab>(m, "WordVocab")
      .def(py::init<std::string, Index_T, Index_T, Index_T, Index_T,
	   std::string, std::string, std::string, std::string, const TokenList_T&>(),
	   py::arg("vocab_file"),
	   py::arg("pad")=0,
	   py::arg("start")=1,
	   py::arg("end")=2,
	   py::arg("unk")=3,
	   py::arg("pad_str")="<PAD>",
	   py::arg("start_str")="<GO>",
	   py::arg("end_str")="<EOS>",
	   py::arg("unk_str")="<UNK>",
	   py::arg("extra_tokens")=TokenList_T()
	   
	   )
      .def(py::init<const TokenList_T&, Index_T, Index_T, Index_T, Index_T,
	   std::string, std::string, std::string, std::string, const TokenList_T&>(),
	   py::arg("token_list"),
	   py::arg("pad")=0,
	   py::arg("start")=1,
	   py::arg("end")=2,
	   py::arg("unk")=3,
	   py::arg("pad_str")="<PAD>",
	   py::arg("start_str")="<GO>",
	   py::arg("end_str")="<EOS>",
	   py::arg("unk_str")="<UNK>",
	   py::arg("extra_tokens")=TokenList_T()
	   
	   )
      .def(py::init<Counter_T, Index_T, Index_T, Index_T, Index_T,
	   std::string, std::string, std::string, std::string, const TokenList_T&, int>(),
	   py::arg("word_counts"),
	   py::arg("pad")=0,
	   py::arg("start")=1,
	   py::arg("end")=2,
	   py::arg("unk")=3,
	   py::arg("pad_str")="<PAD>",
	   py::arg("start_str")="<GO>",
	   py::arg("end_str")="<EOS>",
	   py::arg("unk_str")="<UNK>",
	   py::arg("extra_tokens")=TokenList_T(),
	   py::arg("min_freq")=0
	   
	   )
      .def("lookup", &WordVocab::lookup)
      .def("compile_vocab", &WordVocab::compile_vocab)
      .def_property_readonly("pad_id", &WordVocab::pad_id)
      .def_property_readonly("start_id", &WordVocab::start_id)
      .def_property_readonly("end_id", &WordVocab::end_id)
      .def_property_readonly("unk_id", &WordVocab::unk_id)
      .def_property_readonly("pad_str", &WordVocab::pad_str)
      .def_property_readonly("start_str", &WordVocab::start_str)
      .def_property_readonly("end_str", &WordVocab::end_str)
      .def_property_readonly("unk_str", &WordVocab::unk_str)
      .def_readonly("special_tokens", &WordVocab::special_tokens)
      .def_readonly("vocab", &WordVocab::vocab)
      .def("apply", &WordVocab::apply)
      ;
    
    py::class_<VocabVectorizer>(m, "VocabVectorizer")
      .def(py::init<Vocab*, const TokenList_T&, const TokenList_T&>(),
	   py::arg("vocab"),
	   py::arg("emit_begin_tok")=TokenList_T(),
	   py::arg("emit_end_tok")=TokenList_T()
	   )
      .def(py::init<Vocab*, const Transform_T&, const TokenList_T&, const TokenList_T&>(),
	   py::arg("vocab"),
	   py::arg("transform"),
	   py::arg("emit_begin_tok")=TokenList_T(),
	   py::arg("emit_end_tok")=TokenList_T()
	   )
      .def("piece_to_id", &VocabVectorizer::piece_to_id)
      .def("convert_to_pieces", &VocabVectorizer::convert_to_pieces,
	   py::arg("tokens")
	   )
      .def("convert_to_ids", &VocabVectorizer::convert_to_ids,
	   py::arg("tokens"),
	   py::arg("max_len")=0
	   )
      .def("convert_to_ids_stack", &VocabVectorizer::convert_to_ids_stack,
	   py::arg("tokens"),
	   py::arg("len")
	   )
      .def("count_pieces", &VocabVectorizer::count_pieces,
	   py::arg("tokens")
	   )

      ;
    py::class_<VocabMapVectorizer>(m, "VocabMapVectorizer")
      .def(py::init<Vocab*, const TokenList_T&, const TokenList_T&, const TokenList_T&, std::string>(),
	   py::arg("vocab"),
	   py::arg("emit_begin_tok")=TokenList_T(),
	   py::arg("emit_end_tok")=TokenList_T(),
	   py::arg("fields")=TokenList_T(),
	   py::arg("delim")="~~"
	   )
      .def(py::init<Vocab*, const Transform_T&, const TokenList_T&, const TokenList_T&, const TokenList_T&, std::string>(),
	   py::arg("vocab"),
	   py::arg("transform"),
	   py::arg("emit_begin_tok")=TokenList_T(),
	   py::arg("emit_end_tok")=TokenList_T(),
	   py::arg("fields")=TokenList_T(),
	   py::arg("delim")="~~"
	   )
      .def("piece_to_id", &VocabMapVectorizer::piece_to_id)
      .def("convert_to_pieces", &VocabMapVectorizer::convert_to_pieces,
	   py::arg("tokens")
	   )
      .def("convert_to_ids", &VocabMapVectorizer::convert_to_ids,
	   py::arg("tokens"),
	   py::arg("max_len")=0
	   )
      .def("count_pieces", &VocabMapVectorizer::count_pieces,
	   py::arg("tokens")
	   )
      ;

    
  };
