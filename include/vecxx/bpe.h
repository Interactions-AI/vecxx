#ifndef __VECXX_BPE_H__
#define __VECXX_BPE_H__

#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include "vecxx/utils.h"

const char *BPE_END_WORD = "</w>";
const size_t BPE_END_WORD_LENGTH = 4;
const char *BPE_DELIM = "@@";
const size_t BPE_DELIM_LENGTH = 2;
const char *PACK_DELIM = "  ";

typedef std::pair<std::string, std::string> TPS_T;
typedef std::unordered_map<std::string, uint32_t> Codes_T;
typedef std::unordered_map<std::string, std::string> RevCodes_T;

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
		    const Vocab_T &vocab,
		    bool is_final) {
    auto it = reversed_codes.find(s);
    if (it == reversed_codes.end()) {
	new_subwords.push_back(s);
	return;
    }
    TPS_T pair = unpack_pair(it->second);
    std::string token1 = pair.first;
    if (vocab.find(token1 + BPE_DELIM) == vocab.end()) {
	_decompose_bpe(token1, new_subwords, reversed_codes, vocab, false);
    } else {
	new_subwords.push_back(token1);
    }
    std::string token2 = pair.second;
    auto query = token2 + BPE_DELIM;
    if (is_final) {
	query = token2.substr(0, token2.size() - BPE_END_WORD_LENGTH);
    }
    if (vocab.find(query) == vocab.end()) {
	_decompose_bpe(token2, new_subwords, reversed_codes, vocab, is_final);
    } else {
	new_subwords.push_back(token2);
    }
}

void _limit_vocab_bpe(const TokenList_T &subwords, TokenList_T &new_subwords,
		      const RevCodes_T &reversed_codes,
		      const Vocab_T &vocab) {
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
	if (vocab.find(query) == vocab.end()) {
	    _decompose_bpe(subword, new_subwords, reversed_codes, vocab, is_final);
	} else {
	    new_subwords.push_back(subword);
	}
    }
}


std::string process_bpe(TokenList_T &subwords,
			const Codes_T &codes,
			const RevCodes_T &reversed_codes,
			const Vocab_T &vocab) {
    // merge subWords as much as possible
    TokenList_T new_subwords;
    while (subwords.size() > 1) {
	// find the best pair
	int best_pair_id = -1;
	auto best_pair = codes.end(); // TODO ugly hack that works
	for (int i = 0; i < (int)(subwords.size() - 1); i++) {
	    auto pair = pack_pair(subwords[i], subwords[i + 1]);
	    auto it = codes.find(pair);
	    int pair_rank = it == codes.end() ? -1 : it->second;
	    if (pair_rank >= 0 && (best_pair_id == -1 || int(best_pair->second) > pair_rank)) {
		best_pair = it;
		best_pair_id = i;
	    }
	}
	// if we cannot merge anything, stop
	if (best_pair_id == -1) {
	    break;
	}
	// otherwise, merge subWords
	bool just_merged = false;
	new_subwords = TokenList_T();
	TPS_T best_pair_key = unpack_pair(best_pair->first);
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
			      const Vocab_T& vocab,
			      const Vocab_T& special_tokens,
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


void read_codes_file(const std::string& infile,
		     Codes_T& codes,
		     RevCodes_T &reversed_codes) {
    std::ifstream f(infile.c_str());
    std::string line;
    while (getline(f, line)) {
	auto splits = split(line);
	auto pair = pack_pair(splits[0], splits[1]);
	std::string concat = splits[0] + splits[1];
	codes[pair] = codes.size();
	reversed_codes[concat] = pair;
    }
}

#endif
