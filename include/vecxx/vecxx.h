#ifndef __VECXX_H__
#define __VECXX_H__

#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <functional>

#include "vecxx/utils.h"
#include "vecxx/bpe.h"


class Vectorizer
{

public:
    Vectorizer() {}
    virtual ~Vectorizer() {};
    //virtual std::vector<int> get_dims() const = 0;
    virtual TokenList_T convert_to_pieces(const TokenList_T& tokens) const = 0;
    virtual std::tuple<VecList_T, long unsigned int> convert_to_ids(const TokenList_T& tokens, long unsigned int max_len=0) const = 0;

};

void count_pieces(const Vectorizer& vec,
		  const TokenList_T& tokens, Counter_T& counter) {
    for (std::string c : vec.convert_to_pieces(tokens)) {
	counter[c] += 1;
    }
}

class BasicVectorizer : public Vectorizer
{
protected:
    std::string _emit_begin_tok;
    std::string _emit_end_tok;
    Vocab_T* _vocab;
public:
    BasicVectorizer(Vocab_T* vocab = nullptr,
		    std::string emit_begin_tok = "",
		    std::string emit_end_tok = ""
		    ) : _emit_begin_tok(emit_begin_tok), _emit_end_tok(emit_end_tok), _vocab(vocab) {}
    
    virtual TokenList_T get_pieces(const std::string& token) const {
    	return {token};
    }
    virtual int piece_to_id(const std::string& s) const {
    	auto p = _vocab->find(s);
	if (p == _vocab->end()) {
	    return -1;
	}
	return p->second;
	
    }
    
    virtual ~BasicVectorizer() {}
    virtual TokenList_T convert_to_pieces(const TokenList_T& tokens) const {
	TokenList_T pieces;
	//for (auto t : _emit_begin_tok) {
	if (_emit_begin_tok.length())
	    pieces.push_back(_emit_begin_tok);
	//}
	for (auto t : tokens) {
	    for (auto piece : get_pieces(t)) {
	    	pieces.push_back(piece);
	    }
	}
	//for (auto t : _emit_end_tok) {
	//pieces.push_back(t);
	//}
	if (_emit_end_tok.length())
	    pieces.push_back(_emit_end_tok);
	return pieces;
    }
    virtual std::tuple<VecList_T, long unsigned int> convert_to_ids(const TokenList_T& tokens, long unsigned int max_len=0) const {
    	TokenList_T pieces = convert_to_pieces(tokens);
	auto insz = pieces.size();
	if (max_len <= 0) {
	    max_len = insz;
	}
	auto sz = std::min(insz, max_len);
	VecList_T ids(max_len, 0);
	
        for (auto i = 0; i < sz; ++i) {
	    ids[i] = piece_to_id(pieces[i]);
	}
	return {ids, sz};
	
    }
};

struct PredefinedVocab
{
    PredefinedVocab() {}
    virtual ~PredefinedVocab() {}
    virtual int lookup(const std::string& s) const = 0;
    virtual TokenList_T apply(const TokenList_T& tokens) const = 0;
};
struct BPEVocab : PredefinedVocab
{
    Codes_T codes;
    RevCodes_T reversed_codes;
    Vocab_T vocab;
    int unk_id;
    BPEVocab(std::string vocab_file, std::string codes_file,
	     int offset = 4,
	     int unk = 3) : unk_id(unk) {
	read_vocab_file(vocab_file, vocab, offset);
	read_codes_file(codes_file, codes, reversed_codes);

    }
    virtual int lookup(const std::string& s) const {
    	auto p = vocab.find(s);

	if (p == vocab.end()) {
	    return unk_id;
	}

	return p->second;	
    }

    // FIXME: pass return by ref
    virtual TokenList_T apply(const TokenList_T& tokens) const {
	return _apply_bpe_single(tokens,
				 codes,
				 reversed_codes,
				 vocab);
	
    }
};

class VocabVectorizer : public Vectorizer
{
protected:
    std::string _emit_begin_tok;
    std::string _emit_end_tok;
    PredefinedVocab* _vocab;
    int _offset;
public:
    VocabVectorizer(PredefinedVocab* vocab = nullptr,
		    std::string emit_begin_tok = "",
		    std::string emit_end_tok = ""
		    ) : _emit_begin_tok(emit_begin_tok), _emit_end_tok(emit_end_tok), _vocab(vocab) {}
    virtual ~VocabVectorizer() {}
    
    virtual int piece_to_id(const std::string& s) const {
	return _vocab->lookup(s);
    }
    
    virtual TokenList_T convert_to_pieces(const TokenList_T& tokens) const {
	auto pieces = _vocab->apply(tokens);
	if (_emit_begin_tok.length())
	    pieces.insert(pieces.begin(), _emit_begin_tok);
	if (_emit_end_tok.length())
	    pieces.push_back(_emit_end_tok);
	return pieces;
    }
    virtual std::tuple<VecList_T, long unsigned int> convert_to_ids(const TokenList_T& tokens, long unsigned int max_len=0) const {

    	TokenList_T pieces = convert_to_pieces(tokens);
	auto insz = pieces.size();
	if (max_len <= 0) {
	    max_len = insz;
	}
	auto sz = std::min(insz, max_len);
	VecList_T ids(max_len, 0);
        for (auto i = 0; i < sz; ++i) {
	    ids[i] = piece_to_id(pieces[i]);
	}
	return {ids, sz};
	
    }
};


#endif
