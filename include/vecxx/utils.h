#ifndef __VECXX_UTILS_H__
#define __VECXX_UTILS_H__

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <functional>

typedef uint32_t Index_T;

class MapStrStr
{
public:
    MapStrStr() {}
    virtual ~MapStrStr() {}
    virtual std::tuple<bool, std::string> find(const std::string& key) const = 0;
    virtual bool exists(const std::string& key) const = 0;
    virtual size_t size() const = 0;
    virtual size_t max_size() const = 0;
};
class MapStrInt
{
public:
    MapStrInt() {}
    virtual ~MapStrInt() {}
    virtual std::tuple<bool, Index_T> find(const std::string& key) const = 0;
    virtual bool exists(const std::string& key) const = 0;
    virtual size_t size() const = 0;
    virtual size_t max_size() const = 0;
};


class UnorderedMapStrStr : public MapStrStr
{
    std::unordered_map<std::string, std::string> _m;
public:
    typedef typename std::unordered_map<std::string, std::string>::const_iterator const_iterator;
    UnorderedMapStrStr() {
    }
    ~UnorderedMapStrStr() {}
    std::string& operator[](const std::string& key) {
	return _m[key];
    }
    const_iterator begin() const {
	return _m.begin();
    }
    const_iterator end() const {
	return _m.end();
    }
    bool exists(const std::string& key) const {
	auto it = _m.find(key);
	return (it != _m.end());
    }
    std::tuple<bool, std::string> find(const std::string& key) const {
	auto it = _m.find(key);
	if (it == _m.end()) {
	    return std::make_tuple(false, "");
	}
	return std::make_tuple(true, it->second);
    }
    bool empty() const { return _m.empty(); }
    size_t size() const { return _m.size(); }
    size_t max_size() const { return _m.max_size(); }
    
};

class UnorderedMapStrInt : public MapStrInt
{
    std::unordered_map<std::string, Index_T> _m;
public:
    typedef typename std::unordered_map<std::string, Index_T>::const_iterator const_iterator;
    UnorderedMapStrInt() {
    }
    ~UnorderedMapStrInt() {}
    Index_T& operator[](const std::string& key) {
	return _m[key];
    }
    const_iterator begin() const {
	return _m.begin();
    }
    const_iterator end() const {
	return _m.end();
    }
    std::tuple<bool, Index_T> find(const std::string& key) const {
	auto it = _m.find(key);
	if (it == _m.end()) {
  	    return std::make_tuple(false, 0);
	}
	return std::make_tuple(true, it->second);
    }
    bool exists(const std::string& key) const {
	auto it = _m.find(key);
	return (it != _m.end());
    }

    bool empty() const { return _m.empty(); }
    size_t size() const { return _m.size(); }
    size_t max_size() const { return _m.max_size(); }
    
};

typedef std::vector<std::string> TokenList_T;
typedef std::vector<std::vector<std::string> > ListTokenList_T;
typedef std::unordered_map<std::string, std::string> TokenMap_T;
typedef std::vector<TokenMap_T > TokenMapList_T;
typedef std::map<std::string, int> Counter_T;
typedef std::unordered_map<std::string, uint32_t> SpecialVocab_T;
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

MapStrInt* read_vocab_file(const std::string& infile, int offset=4)
{
    std::ifstream f(infile.c_str());
    std::string line;
    int i = 0;
    UnorderedMapStrInt* vocab = new UnorderedMapStrInt();
    while (getline(f, line)) {
	auto vecs = split(line);
	auto token = vecs[0];
	(*vocab)[token] = i + offset;
	++i;

	
    }
    return vocab;
}

#endif
