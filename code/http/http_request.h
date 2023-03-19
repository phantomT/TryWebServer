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

using std::string;

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

    string &Path();

    string GetMethod() const;

    string GetVersion() const;

    string GetPost(const string &key) const;

    string GetPost(const char *key) const;

    bool IsKeepAlive() const;

private:
    bool ParseRequestLine(const string &line);

    void ParseHeader(const string &line);

    void ParseBody(const string &line);

    void ParsePath();

    void ParsePost();

    void ParseFromUrlEncoded();

    static bool UserVerify(const string &name, const string &pwd, bool isLogin);

    PARSE_STATE h_state;
    string h_method, h_path, h_version, h_body;
    std::unordered_map<string, string> h_header;
    std::unordered_map<string, string> h_post;

    static const std::unordered_set<string> DEFAULT_HTML;
    static const std::unordered_map<string, int> DEFAULT_HTML_TAG;
};

#endif //HTTP_REQUEST_H