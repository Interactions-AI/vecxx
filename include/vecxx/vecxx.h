#ifndef __VECXX_H__
#define __VECXX_H__

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cassert>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <exception>
#include <chrono>

#include "vecxx/utils.h"
#include "vecxx/bpe.h"

/*!
 *  Create a memory-mapped perfect hash map, no offset can be applied
 *  to the indices, as this wouldve been applied during the initia
 *  compilation
 *
 */
MapStrInt* read_vocab_mmap(const std::string& dir) {
    auto c = new PerfectHashMapStrInt(file_in_dir(dir, "ph-vocab"));
    return c;
}
MapStrInt* read_vocab_file(const std::string& infile, int offset=4)
{
    if (is_dir(infile)) {
	return read_vocab_mmap(infile);
    }
    std::ifstream f(infile.c_str());
    if (!f.is_open()) {
        throw std::runtime_error(std::string("No file: ") + infile);
    }
    std::string line;
    int i = 0;
    UnorderedMapStrInt* vocab = new UnorderedMapStrInt();
    while (getline(f, line)) {
	auto vecs = split(line);
	auto token = vecs[0];
	(*vocab)[token] = i + offset;
	++i;

	
    }
    f.close();
    return vocab;
}

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

    virtual std::tuple<VecList_T, VecList_T> convert_to_ids_stack(const ListTokenList_T& list_tokens, long unsigned int len) const = 0;
    
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
        auto t = token.find(fields[0])->second;
	return t;
    }
    
    std::ostringstream oss;
    for (auto i = 0; i < sz - 1; ++i) {
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
    virtual Index_T lookup(const std::string& s, const Transform_T& transform) const = 0;
    virtual TokenList_T apply(const TokenList_T& tokens, const Transform_T& transform) const = 0;
    virtual Index_T pad_id() const = 0;
    virtual Index_T start_id() const = 0;
    virtual Index_T end_id() const = 0;
    virtual Index_T unk_id() const = 0;
    virtual std::string pad_str() const = 0;
    virtual std::string start_str() const = 0;
    virtual std::string end_str() const = 0;
    virtual std::string unk_str() const = 0;
    virtual void compile_vocab(const std::string& target_dir) const = 0;
    virtual std::string rlookup(const Index_T&) const = 0;

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
    MapStrInt* vocab;
    SpecialVocab_T special_tokens;
    WordVocab(std::string vocab_file,
	      Index_T pad = 0,
	      Index_T start = 1,
	      Index_T end = 2,
	      Index_T unk = 3,
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
	_offset = std::max<uint32_t>({pad, start, end, unk}) + 1;
	for (auto token : extra_tokens) {
	    special_tokens[token] = _offset;
	    ++_offset;
	}
	   
	vocab = read_vocab_file(vocab_file, _offset);
    }
    WordVocab(const TokenList_T& vocab_list,
	      Index_T pad = 0,
	      Index_T start = 1,
	      Index_T end = 2,
	      Index_T unk = 3,
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
	auto v = new UnorderedMapStrInt();
	_offset = std::max<uint32_t>({pad, start, end, unk}) + 1;
	for (auto token : extra_tokens) {
	    special_tokens[token] = _offset;
	    ++_offset;
	}
	   
	for (auto token : vocab_list) {
	    (*v)[token] = _offset;
	    ++_offset;
	}
	vocab = v;
	
    }

    WordVocab(const Counter_T& word_counts,
	      Index_T pad = 0,
	      Index_T start = 1,
	      Index_T end = 2,
	      Index_T unk = 3,
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
	_offset = std::max<uint32_t>({pad, start, end, unk}) + 1;
	auto v = new UnorderedMapStrInt();
	for (auto token : extra_tokens) {
	    special_tokens[token] = _offset;
	    ++_offset;
	}
	   
	for (auto kv : word_counts) {
	    if (kv.second > min_freq) {
		(*v)[kv.first] = _offset;
		++_offset;
	    }
	}
	vocab = v;
	
    }

    virtual ~WordVocab() {
	delete vocab;
    }
    virtual void compile_vocab(const std::string& target_dir) const
    {
	compile_str_int( (UnorderedMapStrInt&)(*vocab), join_path(target_dir, "ph-vocab"));
    }

    virtual Index_T pad_id() const { return _pad_id; }
    virtual Index_T start_id() const { return _start_id; }
    virtual Index_T end_id() const { return _end_id; }
    virtual Index_T unk_id() const { return _unk_id; }
    virtual std::string pad_str() const { return _pad_str; }
    virtual std::string start_str() const { return _start_str; }
    virtual std::string end_str() const { return _end_str; }
    virtual std::string unk_str() const { return _unk_str; }
    
    virtual Index_T lookup(const std::string& s, const Transform_T& transform) const {
	auto p = special_tokens.find(s);
	if (p != special_tokens.end()) {
	    return p->second;
	}
	auto t = transform(s);
	bool found;
	Index_T x;
	std::tie(found, x) = vocab->find(t);
	if (!found) {
	    return _unk_id;
	}

	return x;
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

    virtual std::string rlookup(const Index_T& id) const {
        throw std::logic_error("WordVocab::rlookup not implemented");
    }

    
};

class BPEVocab : public Vocab
{
protected:
    Codes_T* _codes;
    RevCodes_T* _reversed_codes;
    Index_T _pad_id;
    Index_T _start_id;
    Index_T _end_id;
    Index_T _unk_id;
    Index_T _offset;
    std::string _pad_str;
    std::string _start_str;
    std::string _end_str;
    std::string _unk_str;
public:
    MapStrInt* vocab;
    SpecialVocab_T special_tokens;
    BPEVocab(std::string vocab_file,
	     std::string codes_file,
	     Index_T pad = 0,
	     Index_T start = 1,
	     Index_T end = 2,
	     Index_T unk = 3,
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
	_offset = std::max<uint32_t>({pad, start, end, unk}) + 1;
	for (auto token : extra_tokens) {
	    special_tokens[token] = _offset;
	    ++_offset;
	}
	vocab = read_vocab_file(vocab_file, _offset);
	read_codes_file(codes_file, _codes, _reversed_codes);
    }
    virtual ~BPEVocab() {
	delete vocab;
	delete _codes;
	delete _reversed_codes;
    }
    virtual Index_T pad_id() const { return _pad_id; }
    virtual Index_T start_id() const { return _start_id; }
    virtual Index_T end_id() const { return _end_id; }
    virtual Index_T unk_id() const { return _unk_id; }
    virtual std::string pad_str() const { return _pad_str; }
    virtual std::string start_str() const { return _start_str; }
    virtual std::string end_str() const { return _end_str; }
    virtual std::string unk_str() const { return _unk_str; }
    
    virtual void compile_vocab(const std::string& target_dir) const
    {
	if (!file_exists(target_dir)) {
	    make_dir(target_dir);
	}
	auto vocab_file = join_path(target_dir, "ph-vocab");
	compile_str_int((UnorderedMapStrInt&)(*vocab),
			vocab_file);
	auto codes_file = join_path(target_dir, "ph-codes");
	
	compile_str_int((const UnorderedMapStrInt&)(*_codes),
			codes_file);
	auto rcodes_file = join_path(target_dir, "ph-rcodes");
	compile_str_str((const UnorderedMapStrStr&)(*_reversed_codes),
			rcodes_file);
    }
    virtual Index_T lookup(const std::string& s, const Transform_T& transform) const {
	auto p = special_tokens.find(s);
	if (p != special_tokens.end()) {
	    return p->second;
	}
	auto t = transform(s);
	bool found;
	Index_T x;
	std::tie(found, x) = vocab->find(t);
	if (!found) {
	    return _unk_id;
	}

	return x;	
    }

    virtual std::string rlookup(const Index_T& idx) const {
        bool found;
        std::string rv;
        std::tie(found, rv) = vocab->rfind(idx);
        if (!found) {
            return "";
        }
        return rv;
    }

    // FIXME: pass return by ref
    virtual TokenList_T apply(const TokenList_T& tokens, const Transform_T& transform) const {
	return _apply_bpe_single(tokens,
				 *_codes,
				 *_reversed_codes,
				 *vocab,
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
	auto sz = std::min<long unsigned int>(insz, max_len);
	VecList_T ids(max_len, _vocab->pad_id());
        for (auto i = 0; i < sz; ++i) {
	    ids[i] = piece_to_id(pieces[i]);
	}
	return std::make_tuple(ids, sz);
	
    }
    virtual std::tuple<VecList_T, VecList_T> convert_to_ids_stack(const ListTokenList_T& list_tokens, long unsigned int len) const {

	auto n = list_tokens.size();
	VecList_T ids(len * n, _vocab->pad_id());
	VecList_T lengths(n, 0);
	for (auto i = 0; i < n; ++i) {
	    TokenList_T pieces = convert_to_pieces(list_tokens[i]);
	    auto insz = std::min<long unsigned int>(len, pieces.size());
	    lengths[i] = (int)insz;
	    for (auto j = 0; j < insz; ++j) {
		ids[i * len + j] = piece_to_id(pieces[j]);
	    }
	}
	return std::make_tuple(ids, lengths);
    }

    std::string decode(const VecList_T& ids) const {
        // reverse look up each ids
        std::vector<std::string> tokens;
        for (const auto & id : ids) {
            tokens.push_back(this->_vocab->rlookup(id));
        }
        std::string rv;
        std::string separator = "@@";
        for (auto & s : tokens) {
            auto found_pos = s.find(separator);
            if (found_pos != std::string::npos) {
                s.replace(found_pos, 2, "");
                rv = rv + s;
                continue;
            }
            rv += s + " ";
        }
        return ltrim(rtrim(rv));
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
	for (auto map_token : map_tokens) {
	    auto t = map_token_to_str(map_token, _fields, _delim);
	    token_list.push_back(t);
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
	auto sz = std::min<long unsigned int>(insz, max_len);
	VecList_T ids(max_len, _vocab->pad_id());
        for (auto i = 0; i < sz; ++i) {
	    ids[i] = piece_to_id(pieces[i]);
	}
	return std::make_tuple(ids, sz);
	
    }
};


#endif
