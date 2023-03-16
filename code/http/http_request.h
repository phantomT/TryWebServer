#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <cerrno>
#include <mysql/mysql.h>

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sql_conn_pool.h"
#include "../pool/sql_conn_RAII.h"

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    HttpRequest() { Init(); }

    void Init();

    bool Parse(Buffer &buff);

    std::string Path() const;

    std::string &Path();

    std::string Method() const;

    std::string Version() const;

    std::string GetPost(const std::string &key) const;

    std::string GetPost(const char *key) const;

    bool IsKeepAlive() const;

private:
    bool ParseRequestLine(const std::string &line);

    void ParseHeader(const std::string &line);

    void ParseBody(const std::string &line);

    void ParsePath();

    void ParsePost();

    void ParseFromUrlEncoded();

    static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);

    PARSE_STATE h_state;
    std::string h_method, h_path, h_version, h_body;
    std::unordered_map<std::string, std::string> h_header;
    std::unordered_map<std::string, std::string> h_post;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

    static int ConvertHex(char ch);
};

#endif //HTTP_REQUEST_H