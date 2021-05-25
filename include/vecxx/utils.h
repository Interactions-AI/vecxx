#ifndef __VECXX_UTILS_H__
#define __VECXX_UTILS_H__

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <functional>

typedef std::vector<std::string> TokenList_T;
typedef std::unordered_map<std::string, std::string> TokenMap_T;
typedef std::vector<TokenMap_T > TokenMapList_T;
typedef std::map<std::string, int> Counter_T;
typedef std::unordered_map<std::string, int> Vocab_T;
typedef std::vector<int> VecList_T;
typedef std::function<std::string(std::string)> Transform_T;

TokenList_T split(const std::string& s,
		  std::string splitter=" ")
{
    TokenList_T vec;
    const auto str_l = s.length();
    const auto split_l = splitter.length();
    size_t pos = 0;
    while (pos < str_l)
    {
        auto nextPos = s.find(splitter, pos);
        if (nextPos == std::string::npos)
            nextPos = str_l;
        if (nextPos != pos)
            vec.push_back(s.substr(pos, nextPos - pos));
        pos = nextPos + split_l;
    }

    if (pos < str_l)
        vec.push_back(s.substr(pos));

    return vec;
}

bool ends_with(const std::string & s, const std::string & match)
{
    const size_t mLen = match.length();
    const size_t sLen = s.length();
    for (size_t i = 0; i < sLen && i < mLen; ++i)
        if (!(s[sLen - i - 1] == match[mLen - i - 1]))
            return false;
    return sLen >= mLen;
}

bool starts_with(const std::string & s, const std::string & match)
{
    const size_t mLen = match.length();
    const size_t sLen = s.length();
    for (size_t i = 0; i < sLen && i < mLen; ++i)
        if (!(s[i] == match[i]))
            return false;
    return sLen >= mLen;
}
void lower(std::string& data)
{
    std::transform(data.begin(), data.end(), data.begin(),
		   [](unsigned char c){ return std::tolower(c); });
}

void upper(std::string& data)
{
    std::transform(data.begin(), data.end(), data.begin(),
		   [](unsigned char c){ return std::toupper(c); });
}

void read_vocab_file(const std::string& infile, Vocab_T& vocab, int offset=4) {
    std::ifstream f(infile.c_str());
    std::string line;
    int i = 0;
    while (getline(f, line)) {
	auto vecs = split(line);
	auto token = vecs[0];
	vocab[token] = i + offset;
	++i;

	
    }
}

#endif
