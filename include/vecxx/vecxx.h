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
    TokenList_T _emit_begin_tok;
    TokenList_T _emit_end_tok;
    Vocab_T* _vocab;
public:
    BasicVectorizer(Vocab_T* vocab,
		    TokenList_T emit_begin_tok,
		    TokenList_T emit_end_tok
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
	TokenList_T pieces(_emit_begin_tok);
	for (auto t : tokens) {
	    for (auto piece : get_pieces(t)) {
	    	pieces.push_back(piece);
	    }
	}
	pieces.insert(pieces.end(), _emit_end_tok.begin(), _emit_end_tok.end());
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

class PredefinedVocab
{
public:
    PredefinedVocab() {}
    virtual ~PredefinedVocab() {}
    virtual int lookup(const std::string& s) const = 0;
    virtual TokenList_T apply(const TokenList_T& tokens) const = 0;
    virtual int pad_id() const = 0;
    virtual int start_id() const = 0;
    virtual int end_id() const = 0;
    virtual int unk_id() const = 0;
    virtual std::string pad_str() const = 0;
    virtual std::string start_str() const = 0;
    virtual std::string end_str() const = 0;
    virtual std::string unk_str() const = 0;
};
class BPEVocab : public PredefinedVocab
{
protected:
    Codes_T _codes;
    RevCodes_T _reversed_codes;
    int _pad_id;
    int _start_id;
    int _end_id;
    int _unk_id;
    std::string _pad_str;
    std::string _start_str;
    std::string _end_str;
    std::string _unk_str;

    int _offset;
public:
    Vocab_T vocab;
    Vocab_T special_tokens;
    BPEVocab(std::string vocab_file, std::string codes_file,
	     int pad = 0,
	     int start = 1,
	     int end = 2,
	     int unk = 3,
	     std::string pad_str = "<PAD>",
	     std::string start_str = "<GO>",
	     std::string end_str = "<EOS>",
	     std::string unk_str = "<UNK>",
	     int offset = 4):
	_pad_id(pad), _start_id(start), _end_id(end), _unk_id(unk), _pad_str(pad_str), _start_str(start_str), _end_str(end_str), _unk_str(unk_str), _offset(offset)  {
	read_vocab_file(vocab_file, vocab, _offset);
	read_codes_file(codes_file, _codes, _reversed_codes);
	special_tokens[_pad_str] = _pad_id;
	special_tokens[_start_str] = _start_id;
	special_tokens[_end_str] = _end_id;
	special_tokens[_unk_str] = _unk_id;
    }
    virtual ~BPEVocab() {}
    virtual int pad_id() const { return _pad_id; }
    virtual int start_id() const { return _start_id; }
    virtual int end_id() const { return _end_id; }
    virtual int unk_id() const { return _unk_id; }
    virtual std::string pad_str() const { return _pad_str; }
    virtual std::string start_str() const { return _start_str; }
    virtual std::string end_str() const { return _end_str; }
    virtual std::string unk_str() const { return _unk_str; }
    
    
    virtual int lookup(const std::string& s) const {
	auto p = special_tokens.find(s);
	if (p != vocab.end()) {
	    return p->second;
	}
	p = vocab.find(s);
	if (p == vocab.end()) {
	    return _unk_id;
	}

	return p->second;	
    }

    // FIXME: pass return by ref
    virtual TokenList_T apply(const TokenList_T& tokens) const {
	return _apply_bpe_single(tokens,
				 _codes,
				 _reversed_codes,
				 vocab);
	
    }
};

class VocabVectorizer : public Vectorizer
{
protected:
    TokenList_T _emit_begin_tok;
    TokenList_T _emit_end_tok;
    PredefinedVocab* _vocab;
public:
    VocabVectorizer(PredefinedVocab* vocab,
		    TokenList_T emit_begin_tok,
		    TokenList_T emit_end_tok
		    ) : _emit_begin_tok(emit_begin_tok), _emit_end_tok(emit_end_tok), _vocab(vocab) {}
    virtual ~VocabVectorizer() {}
    
    virtual int piece_to_id(const std::string& s) const {
	return _vocab->lookup(s);
    }
    
    virtual TokenList_T convert_to_pieces(const TokenList_T& tokens) const {
	auto pieces = _vocab->apply(tokens);
	pieces.insert(pieces.begin(), _emit_begin_tok.begin(), _emit_begin_tok.end());
	pieces.insert(pieces.end(), _emit_end_tok.begin(), _emit_end_tok.end());
	return pieces;

    }
    virtual std::tuple<VecList_T, long unsigned int> convert_to_ids(const TokenList_T& tokens, long unsigned int max_len=0) const {

    	TokenList_T pieces = convert_to_pieces(tokens);
	auto insz = pieces.size();
	if (max_len <= 0) {
	    max_len = insz;
	}
	auto sz = std::min(insz, max_len);
	VecList_T ids(max_len, _vocab->pad_id());
        for (auto i = 0; i < sz; ++i) {
	    ids[i] = piece_to_id(pieces[i]);
	}
	return {ids, sz};
	
    }
};


#endif
