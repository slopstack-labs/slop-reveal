#pragma once
#include <string>
#include <map>

struct HttpResponse {
    int status_code = 0;
    std::string body;
};

class HttpClient {
public:
    explicit HttpClient(int timeout_seconds = 120);
    ~HttpClient();

    HttpResponse post(const std::string& url,
                      const std::string& body,
                      const std::map<std::string, std::string>& headers);

private:
    int timeout_;
};
