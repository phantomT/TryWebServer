#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse();

    ~HttpResponse();

    void Init(const std::string &_srcDir, std::string &_path, bool _isKeepAlive = false, int _code = -1);

    void MakeResponse(Buffer &buff);

    void UnmapFile();

    char *File();

    size_t FileLen() const;

    void ErrorContent(Buffer &buff, const std::string& message) const;

    int Code() const { return h_code; }

private:
    void AddStateLine(Buffer &buff);

    void AddHeader(Buffer &buff);

    void AddContent(Buffer &buff);

    void ErrorHtml();

    std::string GetFileType();

    int h_code;
    bool isKeepAlive;

    std::string path;
    std::string srcDir;

    char *mmFile;
    struct stat mmFileStat;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};


#endif //HTTP_RESPONSE_H