#include "http/http_client.h"
#include <curl/curl.h>
#include <stdexcept>

static std::size_t write_callback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}

HttpClient::HttpClient(int timeout_seconds) : timeout_(timeout_seconds) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

HttpClient::~HttpClient() {
    curl_global_cleanup();
}

HttpResponse HttpClient::post(const std::string& url,
                               const std::string& body,
                               const std::map<std::string, std::string>& headers) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to initialize curl.");

    HttpResponse resp;
    curl_slist* hlist = nullptr;

    for (const auto& [k, v] : headers)
        hlist = curl_slist_append(hlist, (k + ": " + v).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hlist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout_));
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_slist_free_all(hlist);
        curl_easy_cleanup(curl);
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    resp.status_code = static_cast<int>(http_code);

    curl_slist_free_all(hlist);
    curl_easy_cleanup(curl);
    return resp;
}
