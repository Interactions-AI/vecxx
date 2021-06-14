#ifndef __VECXX_BPE_H__
#define __VECXX_BPE_H__

#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include "vecxx/utils.h"
#include "vecxx/iox.h"
#include "vecxx/phf.h"
const char *BPE_END_WORD = "</w>";
const size_t BPE_END_WORD_LENGTH = 4;
const char *BPE_DELIM = "@@";
const size_t BPE_DELIM_LENGTH = 2;
const char *PACK_DELIM = "  ";

typedef std::pair<std::string, std::string> TPS_T;
typedef MapStrInt Codes_T;
typedef MapStrStr RevCodes_T;

// TODO: return tuple of data, fd
uint32_t* _read_uint32s(std::string fname, int sz) {
    void* data = NULL;
    Handle_T fd = 0;
    std::tie(data, fd) = mmap_read(fname, sz*4);
    uint32_t* _d = reinterpret_cast<uint32_t*>(data);
    return _d;
}
std::tuple<char*, uint32_t> _read_chars(std::string fname) {
    uint32_t n = (uint32_t)file_size(fname);
    void* data = NULL;
    Handle_T fd = 0;
    std::tie(data, fd) = mmap_read(fname, n);
    char* _d = reinterpret_cast<char*>(data);
    return std::make_tuple(_d, n);
}
// TODO: do propery memory unmapping
class PerfectHashMapStrStr : public MapStrStr
{
    phf _phf;
    uint32_t* _k;
    uint32_t* _offsets;
    const char* _data;
    uint32_t _data_len;
    uint32_t _hash_key(const std::string& k) const {
	return phf_round32(k, 1337);
    }
public:
    PerfectHashMapStrStr(const std::string& dir)
	: _k(NULL), _offsets(NULL), _data(NULL), _data_len(0) {
	load_phf(_phf, dir);
	_offsets = _read_uint32s(file_in_dir(dir, "offsets.dat"), _phf.m);
	_k = _read_uint32s(file_in_dir(dir, "hkey.dat"), _phf.m);
	std::tie(_data, _data_len) = _read_chars(file_in_dir(dir, "flat.dat"));
    }
    ~PerfectHashMapStrStr() {
	//delete_array(_k);
	//delete_array(_offsets);
	//delete_array(_data);
	
    }
    
    bool exists(const std::string& key) const {
	phf_hash_t idx = PHF::hash(&_phf, key);
	auto offset = _offsets[idx];
	if (offset > _data_len) {
	    return false;
	}
	if (_k[idx] == _hash_key(key)) {
	    return true;
	}
	return false;
    }

    std::tuple<bool, std::string> find(const std::string& key) const
    {
	phf_hash_t idx = PHF::hash(&_phf, key);
	auto offset = _offsets[idx];
	if (offset > _data_len) {
	    return std::make_tuple(false, "");
	}
	// This is a second independent check against false-positives
	// it definitely slows down the code though
	if (_k[idx] == _hash_key(key)) {
	    return std::make_tuple(true, std::string(&_data[offset]));
	}
	return std::make_tuple(false, "");
    }
    size_t size() const { return _phf.m; }
    size_t max_size() const { return _phf.m; }
    
};


class PerfectHashMapStrInt : public MapStrInt
{
    phf _phf;
    uint32_t* _k;
    uint32_t* _v;
    uint32_t _hash_key(const std::string& k) const {
	return phf_round32(k, 1337);
    }
public:
    PerfectHashMapStrInt(const std::string& dir) : _k(NULL), _v(NULL) {
	load_phf(_phf, dir);
	_k = _read_uint32s(file_in_dir(dir, "hkey.dat"), _phf.m);
	_v = _read_uint32s(file_in_dir(dir, "v.dat"), _phf.m);
	
    }
    ~PerfectHashMapStrInt() {
	//delete_array(_v);
	//delete_array(_k);
    }
    bool exists(const std::string& key) const {
	phf_hash_t idx = PHF::hash(&_phf, key);
	return (_k[idx] == _hash_key(key));
    }

    std::tuple<bool, Index_T> find(const std::string& key) const {
	phf_hash_t idx = PHF::hash(&_phf, key);
        const uint32_t p = _v[idx];
	if (_k[idx] == _hash_key(key)) {
	    return std::make_tuple(true, (Index_T)p);
	}
	return std::make_tuple(false, (Index_T)0);
    }
    size_t size() const { return _phf.m; }
    size_t max_size() const { return _phf.m; }
    
};


TPS_T unpack_pair(const std::string& s) {
    auto pos = s.find(PACK_DELIM);
    return std::make_pair<std::string, std::string>(s.substr(0, pos), s.substr(pos + 2));
}
std::string pack_pair(const std::string& s1, const std::string& s2) {
    return s1 + std::string(PACK_DELIM) + s2;
}
std::string pack_pair(const TPS_T& pair) {
    return pack_pair(pair.first, pair.second);
}

void _decompose_bpe(const std::string s,
		    TokenList_T &new_subwords,
		    const RevCodes_T &reversed_codes,
		    const MapStrInt &vocab,
		    bool is_final) {


    bool present;
    std::string pair_value;
    std::tie(present, pair_value) = reversed_codes.find(s);
    if (!present) {
	new_subwords.push_back(s);
	return;
    }
    TPS_T pair = unpack_pair(pair_value);
    std::string token1 = pair.first;
    bool found = vocab.exists(token1 + BPE_DELIM);
    if (!found) {
	_decompose_bpe(token1,
		       new_subwords, reversed_codes, vocab, false);
    }
    else {
	new_subwords.push_back(token1);
    }
    std::string token2 = pair.second;
    auto query = token2 + BPE_DELIM;
    if (is_final) {
	query = token2.substr(0, token2.size() - BPE_END_WORD_LENGTH);
    }
    found = vocab.exists(query);
    if (!found) {
	_decompose_bpe(token2, new_subwords, reversed_codes, vocab, is_final);
    }
    else {
	new_subwords.push_back(token2);
    }
}

void _limit_vocab_bpe(const TokenList_T &subwords, TokenList_T &new_subwords,
		      const RevCodes_T &reversed_codes,
		      const MapStrInt &vocab) {
    std::string query;
    int sz = (int)subwords.size();
    for (int i = 0; i < sz; i++) {
	bool is_final = i == (sz - 1);
	auto &subword = subwords[i];
	if (is_final) {
	    query = subword.substr(0, subword.size() - BPE_END_WORD_LENGTH);
	} else {
	    query = subword + BPE_DELIM;
	}
	bool found = vocab.exists(query);
	if (!found) {
	    _decompose_bpe(subword, new_subwords, reversed_codes, vocab, is_final);
	} else {
	    new_subwords.push_back(subword);
	}
    }
}


std::string process_bpe(TokenList_T &subwords,
			const Codes_T &codes,
			const RevCodes_T &reversed_codes,
			const MapStrInt &vocab) {
    // merge subWords as much as possible
    TokenList_T new_subwords;
    while (subwords.size() > 1) {
	// find the best pair
	uint32_t best_pair_id = 0;
	uint32_t best_pair_rank = 0;
	std::string best_key = "";
	bool have_pair = false;
	//auto best_pair = codes.end(); // TODO ugly hack that works
	
	for (int i = 0; i < (int)(subwords.size() - 1); i++) {
	    auto pair = pack_pair(subwords[i], subwords[i + 1]);
	    uint32_t pair_rank;
	    bool found;

	    std::tie(found, pair_rank) = codes.find(pair);
	    if (found &&  (!have_pair || best_pair_rank > pair_rank)) {
		best_pair_id = i;
		best_key = pair;
		best_pair_rank = pair_rank;
		have_pair = true;
	    }
	}
	// if we cannot merge anything, stop
	if (!have_pair) {
	    break;
	}
	// otherwise, merge subWords
	bool just_merged = false;
	new_subwords = TokenList_T();
	TPS_T best_pair_key = unpack_pair(best_key);
	for (size_t i = 0; i < subwords.size(); i++) {
	    if ((i + 1 < subwords.size()) && (!just_merged) &&
		subwords[i] == best_pair_key.first &&
		subwords[i + 1] == best_pair_key.second) {
		new_subwords.push_back(subwords[i] + subwords[i + 1]);
		just_merged = true;
	    }
	    else {
		if (!just_merged) {
		    new_subwords.push_back(subwords[i]);
		}
		just_merged = false;
	    }
	}
	subwords = new_subwords;
    }
    // check that we are only using words in the dictionary
    if (vocab.size() > 0) {
	TokenList_T new_subwords;
	_limit_vocab_bpe(subwords, new_subwords, reversed_codes, vocab);
	subwords = new_subwords;
    }
    // concat subWords
    std::string result;
    for (auto x : subwords) {
	result = result + x + BPE_DELIM + " ";
    }
    return result.substr(
	0,
	result.size() - BPE_END_WORD_LENGTH - BPE_DELIM_LENGTH - 1 // "</w>@@ "
	);
}


// TODO: make this more efficient by changing process_bpe
TokenList_T _apply_bpe_single(const TokenList_T& s,
			      const Codes_T& codes,
			      const RevCodes_T& reversed_codes,
			      const MapStrInt& vocab,
			      const SpecialVocab_T& special_tokens,
			      const Transform_T& transform) {
    std::string cur;
    TokenList_T words = s;
    int sz = (int)words.size();
    for (int i = 0; i < sz; i++) {
	auto word = words[i];
	auto it = special_tokens.find(word);
	if (it != special_tokens.end()) {
	    cur += " " + word;
	    if (i < sz - 1) cur += " ";
	    continue;
	}
	word = transform(word);
	TokenList_T word_bpes;
	int pos = 0, real_length = 0;
	int last_start = 0;
	while (word[pos]) {
	    bool new_char = (word[pos] & 0xc0) != 0x80;
	    real_length += new_char;
	    if (new_char && pos > 0) {
		auto new_token = word.substr(last_start, pos - last_start);
		word_bpes.push_back(new_token);
		last_start = pos;
	    }
	    pos++;
	}
	auto bpe = word.substr(last_start, std::string::npos) + BPE_END_WORD;
	word_bpes.push_back(bpe);
	cur += process_bpe(word_bpes, codes, reversed_codes, vocab);
	if (i < sz - 1) cur += " ";
    }
    return split(cur);
}

void read_codes_mmap(const std::string& dir, Codes_T*& codes, RevCodes_T*& rev_codes) {
    auto c = new PerfectHashMapStrInt(file_in_dir(dir, "ph-codes"));
    auto rc = new PerfectHashMapStrStr(file_in_dir(dir, "ph-rcodes"));
    codes = c;
    rev_codes = rc;

}
void read_codes_file(const std::string& infile, Codes_T*& codes, RevCodes_T*& rev_codes)
{
    if (is_dir(infile)) {
	std::cout << "file " << infile << " is a directory.  Assuming mmap" << std::endl;
	read_codes_mmap(infile, codes, rev_codes);
	return;
    }
    auto c = new UnorderedMapStrInt();
    auto rc = new UnorderedMapStrStr();
    std::ifstream f(infile.c_str());
    std::string line;
    while (getline(f, line)) {
	auto splits = split(line);
	auto pair = pack_pair(splits[0], splits[1]);
	std::string concat = splits[0] + splits[1];
	(*c)[pair] = (uint32_t)c->size();
	(*rc)[concat] = pair;
    }
    codes = c;
    rev_codes = rc;
}

#endif
