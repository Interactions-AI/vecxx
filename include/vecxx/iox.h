#ifndef __IOX_H__
#define __IOX_H__

#include <inttypes.h> /* PRIu32 PRIx32 */
#include <stdint.h>   /* UINT32_MAX uint32_t uint64_t */
#include "vecxx/phf.h"

#if defined(WIN32) || defined(_WIN32)
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <sys/mman.h>
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
typedef int Handle_T;
std::tuple<void*, Handle_T> mmap_read(std::string file, uint32_t file_size, bool shared=false)
{
    int fd = open(file.c_str(), O_RDONLY);
    void* p = mmap(NULL,
		   file_size,
		   PROT_READ,
		   shared ? MAP_SHARED : MAP_PRIVATE,
		   fd,
		   0);
    return {p, fd};
}

bool file_exists(const std::string& path) {
    
#if defined(WIN32) || defined(_WIN32)
    const DWORD what = GetFileAttributes(path.c_str());
    
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
	    raise std::exception(err);
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
    const DWORD what = GetFileAttributes(path.c_str());
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
    return (CreateDirectory(path.c_str(), NULL)) ? (true): (false);
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
	std::cout << "Reallocating" << std::endl;
	phf_freearray(hash.g, hash.r);
	hash.r = r;
	phf_calloc(&hash.g, hash.r*4);
    }
    bin.read((char*)hash.g, hash.r*4);
    bin.close();
    
}

#endif
