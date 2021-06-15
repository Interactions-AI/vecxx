#ifndef __IOX_H__
#define __IOX_H__

/*
 * This file contains platform independent system calls and IO.
 * Should work on most systems.  The motivatation for most of this code
 * is support for perfect hashing, which we rely on a single-header fork
 * of PHF for (the header is distributed here as well).
 *
 * To facilitate fast load times, these hashes must be memory mapped, and
 * to compute the perfect hashes, we rely on some platform specific calls,
 * as well as for some filesystem type IO.
 *
 * For this library, we are targeting C++11 or later.
 */ 
#include <inttypes.h> /* PRIu32 PRIx32 */
#include <stdint.h>   /* UINT32_MAX uint32_t uint64_t */
#include "vecxx/phf.h"

#if defined(WIN32) || defined(_WIN32)
#  include <windows.h>
#  define _CRT_RAND_S
#  include <cstdlib>
   typedef HANDLE Handle_T;
#else
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <sys/mman.h>
#  include <unistd.h>
   typedef int Handle_T;
#endif

// This is modified from the CPP file of PHF for windows
static uint32_t randomseed() {
#if defined(WIN32) || defined(_WIN32)
    errno_t err;
    uint32_t seed;
    err = rand_s(&seed);
    if (err != 0) {
	std::cerr << "rand_s failed!" << std::endl;
        return time(0);
    }
    return seed;
#elif __APPLE__
    /*
     * As of macOS 10.13.6 ccaes_vng_ctr_crypt and drbg_update in
     * libcorecrypto.dylib trigger a "Conditional jump or move on
     * uninitialized value(s)". As of Valgrind 3.15.0.GIT (20190214)
     * even when suppressed it still taints code indirectly conditioned
     * on the seed value.
     */
    uint32_t seed = arc4random();
    return seed;
#elif defined BSD /* catchall for modern BSDs, which all have arc4random */
    return arc4random();
#else
    FILE *fp;
    uint32_t seed;
    
    if (!(fp = fopen("/dev/urandom", "r"))) {
	std::cerr << "/dev/urandom access failed!" << std::endl;
	return time(0);
    }
    
    if (1 != fread(&seed, sizeof seed, 1, fp)) {
	std::cerr << "/dev/urandom access failed!" << std::endl;
	fclose(fp);
	return time(0);
    }
    
    fclose(fp);
    
    return seed;
#endif
}



#if defined(WIN32) || defined(_WIN32)
#if defined(_WIN64)
    typedef int64_t Offset_T;
#else
    typedef uint32_t Offset_T;
#endif

// This is modified and simplified from:
// https://github.com/alitrack/mman-win32/blob/master/mman.h
// to fit a simpler read-only use-case
void* mmap_read_win32(void *addr, size_t len, HANDLE h, Offset_T off=0)
{
    HANDLE fm;
    
    void * map = NULL;
    
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4293)
#endif

    const DWORD dwFileOffsetLow = (sizeof(Offset_T) <= sizeof(DWORD)) ?
                    (DWORD)off : (DWORD)(off & 0xFFFFFFFFL);
    const DWORD dwFileOffsetHigh = (sizeof(Offset_T) <= sizeof(DWORD)) ?
                    (DWORD)0 : (DWORD)((off >> 32) & 0xFFFFFFFFL);
    const DWORD protect = PAGE_READONLY;
    const DWORD desiredAccess = PAGE_READONLY;
    const Offset_T maxSize = off + (Offset_T)len;
    const DWORD dwMaxSizeLow = (sizeof(Offset_T) <= sizeof(DWORD)) ?
                    (DWORD)maxSize : (DWORD)(maxSize & 0xFFFFFFFFL);
    const DWORD dwMaxSizeHigh = (sizeof(Offset_T) <= sizeof(DWORD)) ?
                    (DWORD)0 : (DWORD)((maxSize >> 32) & 0xFFFFFFFFL);

#ifdef _MSC_VER
#pragma warning(pop)
#endif

    fm = CreateFileMapping(h, NULL, protect, dwMaxSizeHigh, dwMaxSizeLow, NULL);
    if (fm == NULL)
    {
        return NULL;
    }
    map = MapViewOfFile(fm, desiredAccess, dwFileOffsetHigh, dwFileOffsetLow, len);
    CloseHandle(fm);
    if (map == NULL)
    {
        return NULL;
    }

    return map;
}

int munmap(void *addr, size_t len)
{
    if (UnmapViewOfFile(addr))
        return 0;
    return -1;
}

#endif



std::ifstream::pos_type file_size(std::string filename)
{
    std::ifstream in(filename.c_str(), std::ifstream::ate | std::ifstream::binary);
    return in.tellg(); 
}
const char* path_delimiter()
{
#if defined(WIN32) || defined(_WIN32)
    return "\\";
#else
    return "/";
#endif
}


std::string join_path(std::string p1, std::string p2) {
    return p1 + std::string(path_delimiter()) + p2;
}

std::tuple<void*, Handle_T> mmap_read(std::string file, uint32_t file_size, bool shared=false)
{
    
#if defined(WIN32) || defined(_WIN32)
    HANDLE fd = ::CreateFileA(file.c_str(),
			      GENERIC_READ,
			      FILE_SHARE_READ,
			      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(fd != INVALID_HANDLE_VALUE);
    void* p = mmap_read_win32(NULL, file_size, fd, 0);
#else
    int fd = open(file.c_str(), O_RDONLY);
    void* p = mmap(NULL,
		   file_size,
		   PROT_READ,
		   shared ? MAP_SHARED : MAP_PRIVATE,
		   fd,
		   0);
#endif
    return {p, fd};
    
}
void close_file(Handle_T fd) {
#if defined(WIN32) || defined(_WIN32)
    CloseHandle(fd);
#else
    close(fd);
#endif
}

bool file_exists(const std::string& path) {
    
#if defined(WIN32) || defined(_WIN32)
    const DWORD what = GetFileAttributesA(path.c_str());
    
    if (what == INVALID_FILE_ATTRIBUTES)
    {
	const DWORD errCode = GetLastError();
	if (errCode != ERROR_FILE_NOT_FOUND && errCode != ERROR_PATH_NOT_FOUND)
	{
	    char* err = NULL;
	    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			  FORMAT_MESSAGE_FROM_SYSTEM,
			  NULL, errCode,
			  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			  (LPTSTR) &err, 0, NULL);
	    throw std::exception(err);
	}
	return false;
    }
    
    return true;
#else
    struct stat info;
    if (stat(path.c_str(), &info) == -1)
	return false;
    return true;
#endif
}

bool is_dir(const std::string& path)
{
#if defined(WIN32) || defined(_WIN32)
    const DWORD what = GetFileAttributesA(path.c_str());
    return (what != INVALID_FILE_ATTRIBUTES &&
            (what & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat info;
    if (stat(path.c_str(), &info) == -1)
        return false;
    return (S_ISDIR(info.st_mode)) ? (true) : (false);
#endif
}

bool make_dir(const std::string& path) {
#if defined(WIN32) || defined(_WIN32)
    return (CreateDirectoryA(path.c_str(), NULL)) ? (true): (false);
#else
    
    if (::mkdir(path.c_str(), 0777) == 0)
	return true;
    return false;
#endif
}

std::string file_in_dir(const std::string& dir, const std::string& basename) {
    std::ostringstream out;
    out << dir << path_delimiter() << basename;
    return out.str();    
}

void save_phf(const phf& hash, const std::string& dir) {
    if (!file_exists(dir)) {
	std::cerr << "creating " << dir << std::endl;
	make_dir(dir);
    }
    std::ofstream ofs(file_in_dir(dir, "md.txt"));
    ofs << hash.nodiv << std::endl;
    ofs << hash.seed << std::endl;
    ofs << hash.r << std::endl;
    ofs << hash.m << std::endl;
    ofs << hash.d_max << std::endl;
    ofs << hash.g_op << std::endl;
    std::ofstream bin(file_in_dir(dir, "hash.dat"), std::ios::out | std::ios::binary);
    bin.write((const char*)hash.g, hash.r*4);
    bin.close();
    
}
void load_phf(phf& hash, const std::string& dir) {
    std::ifstream ifs(file_in_dir(dir, "md.txt"));
    size_t r;
    ifs >> hash.nodiv;
    ifs >> hash.seed;
    ifs >> r;
    ifs >> hash.m;
    ifs >> hash.d_max;
    ifs >> hash.g_op;
    std::ifstream bin(file_in_dir(dir, "hash.dat"), std::ios::in | std::ios::binary);
    if (r != hash.r || hash.g == NULL) {
	if (hash.g != NULL) {
	    phf_freearray(hash.g, hash.r);
	}
	hash.r = r;
	phf_calloc(&hash.g, hash.r);
    }
    bin.read((char*)hash.g, hash.r*4);
    bin.close();
    
}

// TODO: dont hardcode the hash
uint32_t _hash_key(const std::string& k, uint32_t h=1337) {
  return phf_round32(k, h);
}

void compile_str_int(const UnorderedMapStrInt& c, std::string dir,size_t alpha=80, size_t lambda=4) {
    size_t n = c.size();
    std::string* k = new std::string[n];
    size_t i = 0;
    for (auto p = c.begin(); p != c.end(); ++p, ++i) {
	k[i] = p->first;
    }
    phf phf;
    uint32_t seed = randomseed();
    PHF::init<std::string, 0>(&phf, k, n, lambda, alpha, seed);
    //PHF::compact(&phf);

    auto m = phf.m;
    save_phf(phf, dir);

    uint32_t* h = new uint32_t[m];
    memset(h, 0, m*4);
    uint32_t* v = new uint32_t[m];
    memset(v, 0, m*4);

    for (auto p = c.begin(); p != c.end(); ++p) {
	phf_hash_t idx = PHF::hash(&phf, p->first);
	h[idx] = _hash_key(p->first);
	v[idx] = (uint32_t)p->second;
	
    }
    std::ofstream bin(file_in_dir(dir, "v.dat"),
		      std::ios::out | std::ios::binary);
    bin.write((const char*)v, m*4);
    bin.close();

    std::ofstream hbin(file_in_dir(dir, "hkey.dat"),
		       std::ios::out | std::ios::binary);
    hbin.write((const char*)h, m*4);
    hbin.close();

    PHF::destroy(&phf);
    delete [] k;
    delete [] h;
    delete [] v;

}

void compile_str_str(const UnorderedMapStrStr& c, std::string dir, size_t alpha=80, size_t lambda=4) {
    size_t n = c.size();
    std::string* k = new std::string[n];
    size_t i = 0;
    for (auto p = c.begin(); p != c.end(); ++p, ++i) {
	k[i] = p->first;
    }
    phf phf;
    uint32_t seed = randomseed();
    PHF::init<std::string, 0>(&phf, k, n, lambda, alpha, seed);
    //PHF::compact(&phf);

    auto m = phf.m;
    save_phf(phf, dir);


    uint32_t* h = new uint32_t[m];
    uint32_t* offsets = new uint32_t[m];
    std::vector<char> flat;
    memset(h, 0, m*4);
    memset(offsets, 0, m*4);
    assert(flat.size() <= UINT32_MAX);
    for (auto p = c.begin(); p != c.end(); ++p) {
	phf_hash_t idx = PHF::hash(&phf, p->first);
	h[idx] = _hash_key(p->first);
	// 4GB limit on values
	offsets[idx] = (uint32_t)flat.size();
	for (char ch : p->second) {
	    flat.push_back(ch);
	}
	flat.push_back(0);
    }
    PHF::init<std::string, 0>(&phf, k, n, lambda, alpha, seed);
    //PHF::compact(&phf);
    save_phf(phf, dir);    
    PHF::destroy(&phf);
    std::ofstream obin(file_in_dir(dir, "offsets.dat"),
		       std::ios::out | std::ios::binary);
    obin.write((const char*)offsets, m*4);
    obin.close();
    std::ofstream hbin(file_in_dir(dir, "hkey.dat"),
		       std::ios::out | std::ios::binary);
    hbin.write((const char*)h, m*4);
    hbin.close();

    std::ofstream cbin(file_in_dir(dir, "flat.dat"),
		       std::ios::out | std::ios::binary);
    cbin.write((const char*)&flat[0], flat.size());
    cbin.close();


    delete [] k;
    delete [] h;
    delete [] offsets;

}

#endif
