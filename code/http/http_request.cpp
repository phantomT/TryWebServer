#include "http_request.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
        "/index", "/register", "/login",
        "/welcome", "/video", "/picture",};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
        {"/register.html", 0},
        {"/login.html",    1},};

void HttpRequest::Init() {
    h_method = h_path = h_version = h_body = "";
    h_state = REQUEST_LINE;
    h_header.clear();
    h_post.clear();
}

bool HttpRequest::IsKeepAlive() const {
    if (h_header.count("Connection") == 1) {
        return h_header.find("Connection")->second == "keep-alive" && h_version == "1.1";
    }
    return false;
}

bool HttpRequest::Parse(Buffer &buff) {
    const char CRLF[] = "\r\n";
    if (buff.ReadableBytes() <= 0) {
        return false;
    }
    while (buff.ReadableBytes() && h_state != FINISH) {
        const char *lineEnd = std::search(buff.Peek(), buff.BeginWriteConst(),
                                          CRLF, CRLF + 2);
        std::string line(buff.Peek(), lineEnd);
        switch (h_state) {
            case REQUEST_LINE:
                if (!ParseRequestLine(line)) {
                    return false;
                }
                ParsePath();
                break;
            case HEADERS:
                ParseHeader(line);
                if (buff.ReadableBytes() <= 2) {
                    h_state = FINISH;
                }
                break;
            case BODY:
                ParseBody(line);
                break;
            default:
                break;
        }
        if (lineEnd == buff.BeginWrite()) { break; }
        buff.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", h_method.c_str(), h_path.c_str(), h_version.c_str())
    return true;
}

void HttpRequest::ParsePath() {
    if (h_path == "/") {
        h_path = "/index.html";
    } else {
        for (auto &item: DEFAULT_HTML) {
            if (item == h_path) {
                h_path += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine(const std::string &line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (regex_match(line, subMatch, patten)) {
        h_method = subMatch[1];
        h_path = subMatch[2];
        h_version = subMatch[3];
        h_state = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error")
    return false;
}

void HttpRequest::ParseHeader(const std::string &line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (regex_match(line, subMatch, patten)) {
        h_header[subMatch[1]] = subMatch[2];
    } else {
        h_state = BODY;
    }
}

void HttpRequest::ParseBody(const std::string &line) {
    h_body = line;
    ParsePost();
    h_state = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size())
}

int HttpRequest::ConvertHex(char ch) {
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch;
}

void HttpRequest::ParsePost() {
    if (h_method == "POST" && h_header["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlEncoded();
        if (DEFAULT_HTML_TAG.count(h_path)) {
            int tag = DEFAULT_HTML_TAG.find(h_path)->second;
            LOG_DEBUG("Tag:%d", tag)
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (UserVerify(h_post["username"], h_post["password"], isLogin)) {
                    h_path = "/welcome.html";
                } else {
                    h_path = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::ParseFromUrlEncoded() {
    if (h_body.empty()) { return; }

    std::string key, value;
    int num = 0;
    int n = h_body.size();
    int i = 0, j = 0;

    for (; i < n; ++i) {
        char ch = h_body[i];
        switch (ch) {
            case '=':
                key = h_body.substr(j, i - j);
                j = i + 1;
                break;
            case '+':
                h_body[i] = ' ';
                break;
            case '%':
                num = ConvertHex(h_body[i + 1]) * 16 + ConvertHex(h_body[i + 2]);
                h_body[i + 2] = num % 10 + '0';
                h_body[i + 1] = num / 10 + '0';
                i += 2;
                break;
            case '&':
                value = h_body.substr(j, i - j);
                j = i + 1;
                h_post[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str())
                break;
            default:
                break;
        }
    }
    assert(j <= i);
    if (h_post.count(key) == 0 && j < i) {
        value = h_body.substr(j, i - j);
        h_post[key] = value;
    }
}

bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool isLogin) {
    if (name.empty() || pwd.empty()) { return false; }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str())
    MYSQL *sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    unsigned int j = 0;
    char order[256] = {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if (!isLogin) { flag = true; }
    /* 查询用户及密码 */
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order)

    if (mysql_query(sql, order)) {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1])
        std::string password(row[1]);
        // 注册行为 且 用户名未被使用
        if (isLogin) {
            if (pwd == password) { flag = true; }
            else {
                flag = false;
                LOG_DEBUG("pwd error!")
            }
        } else {
            flag = false;
            LOG_DEBUG("user used!")
        }
    }
    mysql_free_result(res);

    // 注册行为 且 用户名未被使用
    if (!isLogin && flag) {
        LOG_DEBUG("register!")
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order)
        if (mysql_query(sql, order)) {
            LOG_DEBUG("Insert error!")
            flag = false;
        }
        flag = true;
    }
    SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG("UserVerify success!!")
    return flag;
}

std::string HttpRequest::Path() const {
    return h_path;
}

std::string &HttpRequest::Path() {
    return h_path;
}

std::string HttpRequest::Method() const {
    return h_method;
}

std::string HttpRequest::Version() const {
    return h_version;
}

std::string HttpRequest::GetPost(const std::string &key) const {
    assert(!key.empty());
    if (h_post.count(key) == 1) {
        return h_post.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char *key) const {
    assert(key != nullptr);
    if (h_post.count(key) == 1) {
        return h_post.find(key)->second;
    }
    return "";
}