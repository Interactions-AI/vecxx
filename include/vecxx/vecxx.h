#ifndef __VECXX_H__
#define __VECXX_H__

#include <fstream>
#include <sstream>
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
    virtual int piece_to_id(const std::string& s) const = 0;
    virtual Counter_T count_pieces(const TokenList_T& tokens) {
	Counter_T counter;
	for (std::string c : convert_to_pieces(tokens)) {
	    counter[c] += 1;
	}
	return counter;
    }


};

class MapVectorizer
{

public:
    MapVectorizer() {}
    virtual ~MapVectorizer() {};
    //virtual std::vector<int> get_dims() const = 0;
    virtual TokenList_T convert_to_pieces(const TokenMapList_T& tokens) const = 0;
    virtual std::tuple<VecList_T, long unsigned int> convert_to_ids(const TokenMapList_T& tokens, long unsigned int max_len=0) const = 0;
    virtual int piece_to_id(const std::string& s) const = 0;

    virtual Counter_T count_pieces(const TokenMapList_T& tokens) {
	Counter_T counter;
	for (std::string c : convert_to_pieces(tokens)) {
	    counter[c] += 1;
	}
	return counter;
    }

};

std::string map_token_to_str(const TokenMap_T& token, const TokenList_T& fields, const std::string& delim) {
    int sz = (int)fields.size();
    if (sz == 1) {
	return token.find(fields[0])->second;
    }
    
    std::ostringstream oss;
    for (auto i = 0; i < sz; ++i) {
	oss << token.find(fields[i])->second << delim;
    }
    oss << token.find(fields[sz - 1])->second;
    return oss.str();
}




class Vocab
{
public:
    Vocab() {}
    virtual ~Vocab() {}
    virtual int lookup(const std::string& s, const Transform_T& transform) const = 0;
    virtual TokenList_T apply(const TokenList_T& tokens, const Transform_T& transform) const = 0;
    virtual int pad_id() const = 0;
    virtual int start_id() const = 0;
    virtual int end_id() const = 0;
    virtual int unk_id() const = 0;
    virtual std::string pad_str() const = 0;
    virtual std::string start_str() const = 0;
    virtual std::string end_str() const = 0;
    virtual std::string unk_str() const = 0;
};
class WordVocab : public Vocab
{
protected:
    int _pad_id;
    int _start_id;
    int _end_id;
    int _unk_id;
    int _offset;
    std::string _pad_str;
    std::string _start_str;
    std::string _end_str;
    std::string _unk_str;
public:
    Vocab_T vocab;
    Vocab_T special_tokens;
    WordVocab(std::string vocab_file,
	      int pad = 0,
	      int start = 1,
	      int end = 2,
	      int unk = 3,
	      std::string pad_str = "<PAD>",
	      std::string start_str = "<GO>",
	      std::string end_str = "<EOS>",
	      std::string unk_str = "<UNK>",
	      const TokenList_T& extra_tokens = TokenList_T() ):
	_pad_id(pad),
	_start_id(start),
	_end_id(end),
	_unk_id(unk),
	_pad_str(pad_str),
	_start_str(start_str),
	_end_str(end_str),
	_unk_str(unk_str) {
	special_tokens[_pad_str] = _pad_id;
	special_tokens[_start_str] = _start_id;
	special_tokens[_end_str] = _end_id;
	special_tokens[_unk_str] = _unk_id;
	_offset = std::max({pad, start, end, unk}) + 1;
	for (auto token : extra_tokens) {
	    special_tokens[token] = _offset;
	    ++_offset;
	}
	   
	read_vocab_file(vocab_file, vocab, _offset);
	    
    }
    WordVocab(const TokenList_T& vocab_list,
	      int pad = 0,
	      int start = 1,
	      int end = 2,
	      int unk = 3,
	      std::string pad_str = "<PAD>",
	      std::string start_str = "<GO>",
	      std::string end_str = "<EOS>",
	      std::string unk_str = "<UNK>",
	      const TokenList_T& extra_tokens = TokenList_T(),
	      int min_freq=0):
	_pad_id(pad),
	_start_id(start),
	_end_id(end),
	_unk_id(unk),
	_pad_str(pad_str),
	_start_str(start_str),
	_end_str(end_str),
	_unk_str(unk_str) {
	special_tokens[_pad_str] = _pad_id;
	special_tokens[_start_str] = _start_id;
	special_tokens[_end_str] = _end_id;
	special_tokens[_unk_str] = _unk_id;
	_offset = std::max({pad, start, end, unk}) + 1;
	for (auto token : extra_tokens) {
	    special_tokens[token] = _offset;
	    ++_offset;
	}
	   
	for (auto token : vocab_list) {
	  vocab[token] = _offset;
	  ++_offset;
	}
	
    }

    WordVocab(const Counter_T& word_counts,
	      int pad = 0,
	      int start = 1,
	      int end = 2,
	      int unk = 3,
	      std::string pad_str = "<PAD>",
	      std::string start_str = "<GO>",
	      std::string end_str = "<EOS>",
	      std::string unk_str = "<UNK>",
	      const TokenList_T& extra_tokens = TokenList_T(),
	      int min_freq=0):
	_pad_id(pad),
	_start_id(start),
	_end_id(end),
	_unk_id(unk),
	_pad_str(pad_str),
	_start_str(start_str),
	_end_str(end_str),
	_unk_str(unk_str) {
	special_tokens[_pad_str] = _pad_id;
	special_tokens[_start_str] = _start_id;
	special_tokens[_end_str] = _end_id;
	special_tokens[_unk_str] = _unk_id;
	_offset = std::max({pad, start, end, unk}) + 1;
	for (auto token : extra_tokens) {
	    special_tokens[token] = _offset;
	    ++_offset;
	}
	   
	for (auto kv : word_counts) {
	    if (kv.second > min_freq) {
		vocab[kv.first] = _offset;
		++_offset;
	    }
	}
	
    }

    virtual ~WordVocab() {}
    virtual int pad_id() const { return _pad_id; }
    virtual int start_id() const { return _start_id; }
    virtual int end_id() const { return _end_id; }
    virtual int unk_id() const { return _unk_id; }
    virtual std::string pad_str() const { return _pad_str; }
    virtual std::string start_str() const { return _start_str; }
    virtual std::string end_str() const { return _end_str; }
    virtual std::string unk_str() const { return _unk_str; }
    
    virtual int lookup(const std::string& s, const Transform_T& transform) const {
	auto p = special_tokens.find(s);
	if (p != special_tokens.end()) {
	    return p->second;
	}
	auto t = transform(s);
	p = vocab.find(t);
	if (p == vocab.end()) {
	    return _unk_id;
	}

	return p->second;	
    }


    virtual TokenList_T apply(const TokenList_T& tokens, const Transform_T& transform) const {
	TokenList_T output;
	for (auto s : tokens) {
	    auto p = special_tokens.find(s);
	    if (p != special_tokens.end()) {
		output.push_back(s);
	    }
	    else {
		
		output.push_back(transform(s));
	    }
	}
	return output;
    }
};

class BPEVocab : public Vocab
{
protected:
    Codes_T _codes;
    RevCodes_T _reversed_codes;
    int _pad_id;
    int _start_id;
    int _end_id;
    int _unk_id;
    int _offset;
    std::string _pad_str;
    std::string _start_str;
    std::string _end_str;
    std::string _unk_str;
public:
    Vocab_T vocab;
    Vocab_T special_tokens;
    BPEVocab(std::string vocab_file,
	     std::string codes_file,
	     int pad = 0,
	     int start = 1,
	     int end = 2,
	     int unk = 3,
	     std::string pad_str = "<PAD>",
	     std::string start_str = "<GO>",
	     std::string end_str = "<EOS>",
	     std::string unk_str = "<UNK>",
	     const TokenList_T& extra_tokens = TokenList_T() ):
	_pad_id(pad),
	_start_id(start),
	_end_id(end),
	_unk_id(unk),
	_pad_str(pad_str),
	_start_str(start_str),
	_end_str(end_str),
	_unk_str(unk_str) {
	special_tokens[_pad_str] = _pad_id;
	special_tokens[_start_str] = _start_id;
	special_tokens[_end_str] = _end_id;
	special_tokens[_unk_str] = _unk_id;
	_offset = std::max({pad, start, end, unk}) + 1;
	for (auto token : extra_tokens) {
	    special_tokens[token] = _offset;
	    ++_offset;
	}
	   
	read_vocab_file(vocab_file, vocab, _offset);
	read_codes_file(codes_file, _codes, _reversed_codes);
	    
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
    
    
    virtual int lookup(const std::string& s, const Transform_T& transform) const {
	auto p = special_tokens.find(s);
	if (p != special_tokens.end()) {
	    return p->second;
	}
	auto t = transform(s);
	p = vocab.find(t);
	if (p == vocab.end()) {
	    return _unk_id;
	}

	return p->second;	
    }

    // FIXME: pass return by ref
    virtual TokenList_T apply(const TokenList_T& tokens, const Transform_T& transform) const {
	return _apply_bpe_single(tokens,
				 _codes,
				 _reversed_codes,
				 vocab,
				 special_tokens,
				 transform);
	
    }
};

class VocabVectorizer : public Vectorizer
{
protected:
    Vocab* _vocab;
    Transform_T _transform;
    TokenList_T _emit_begin_tok;
    TokenList_T _emit_end_tok;
public:
    VocabVectorizer(Vocab* vocab,
		    const Transform_T& transform,
		    const TokenList_T& emit_begin_tok = TokenList_T(),
		    const TokenList_T& emit_end_tok = TokenList_T()
	) : _vocab(vocab), _transform(transform), _emit_begin_tok(emit_begin_tok), _emit_end_tok(emit_end_tok) {

    }

    VocabVectorizer(Vocab* vocab,
		    const TokenList_T& emit_begin_tok = TokenList_T(),
		    const TokenList_T& emit_end_tok = TokenList_T()
	) :  _vocab(vocab), _emit_begin_tok(emit_begin_tok), _emit_end_tok(emit_end_tok) {
	_transform = [](std::string s) -> std::string { return s; };
    }
    virtual ~VocabVectorizer() {}
    
    virtual int piece_to_id(const std::string& s) const {
	return _vocab->lookup(s, _transform);
    }
    
    virtual TokenList_T convert_to_pieces(const TokenList_T& tokens) const {
	auto pieces = _vocab->apply(tokens, _transform);
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

class VocabMapVectorizer : public MapVectorizer
{
protected:
    Vocab* _vocab;
    Transform_T _transform;
    TokenList_T _emit_begin_tok;
    TokenList_T _emit_end_tok;
    TokenList_T _fields;
    std::string _delim;
public:
    VocabMapVectorizer(Vocab* vocab,
		       const Transform_T& transform,
		       const TokenList_T& emit_begin_tok = TokenList_T(),
		       const TokenList_T& emit_end_tok = TokenList_T(),
		       const TokenList_T& fields = TokenList_T(),
		       std::string delim="~~"
	) : _vocab(vocab), _transform(transform), _emit_begin_tok(emit_begin_tok), _emit_end_tok(emit_end_tok), _fields(fields), _delim(delim) {
	if (_fields.empty()) {
	    _fields.push_back("text");
	}

    }

    VocabMapVectorizer(Vocab* vocab,
		       const TokenList_T& emit_begin_tok = TokenList_T(),
		       const TokenList_T& emit_end_tok = TokenList_T(),
		       const TokenList_T& fields = TokenList_T(),
		       std::string delim="~~"
	) :  _vocab(vocab), _emit_begin_tok(emit_begin_tok), _emit_end_tok(emit_end_tok), _fields(fields), _delim(delim) {
	_transform = [](std::string s) -> std::string { return s; };
	if (_fields.empty()) {
	    _fields.push_back("text");
	}

    }
    virtual ~VocabMapVectorizer() {}
    
    virtual int piece_to_id(const std::string& s) const {
	return _vocab->lookup(s, _transform);
    }

    virtual void _convert_to_tokens(const TokenMapList_T& map_tokens,
				    TokenList_T& token_list) const {
	assert(tokens.size() == output.size());
	for (auto map_token : map_tokens) {
	    token_list.push_back(map_token_to_str(map_token, _fields, _delim));
	}

    }
    
    virtual TokenList_T convert_to_pieces(const TokenMapList_T& tokens) const {
	TokenList_T token_list;
	_convert_to_tokens(tokens, token_list);
	auto pieces = _vocab->apply(token_list, _transform);
	pieces.insert(pieces.begin(), _emit_begin_tok.begin(), _emit_begin_tok.end());
	pieces.insert(pieces.end(), _emit_end_tok.begin(), _emit_end_tok.end());
	return pieces;

    }
    virtual std::tuple<VecList_T, long unsigned int> convert_to_ids(const TokenMapList_T& tokens, long unsigned int max_len=0) const {

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
