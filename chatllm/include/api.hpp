#ifndef API_HPP
#define API_HPP

#include <string>
#include <vector>
#include <curl/curl.h>
#include <functional>

struct StreamData {
    std::string buffer;
    std::function<void(const std::string&)>* callback;
};

class DeepSeekAPI {
private:
    std::string api_key_;
    std::string model_;
    std::string base_url_;
    
    static size_t streamCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t realsize = size * nmemb;
        StreamData* data = static_cast<StreamData*>(userp);
        data->buffer.append(static_cast<char*>(contents), realsize);
        
        // 逐行处理
        size_t pos = 0;
        while (pos < data->buffer.length()) {
            size_t line_end = data->buffer.find('\n', pos);
            if (line_end == std::string::npos) break;
            
            std::string line = data->buffer.substr(pos, line_end - pos);
            pos = line_end + 1;
            
            if (line.substr(0, 6) == "data: ") {
                std::string json_str = line.substr(6);
                if (json_str == "[DONE]") {
                    return realsize;
                }
                
                try {
                    auto j = nlohmann::json::parse(json_str);
                    if (j.contains("choices") && j["choices"].size() > 0) {
                        auto& delta = j["choices"][0]["delta"];
                        if (delta.contains("content")) {
                            std::string chunk = delta["content"].get<std::string>();
                            if (data->callback && *data->callback) {
                                (*data->callback)(chunk);
                            }
                        }
                    }
                } catch (...) {
                    // 忽略解析错误
                }
            }
        }
        
        // 保留未处理完的数据
        data->buffer = data->buffer.substr(pos);
        return realsize;
    }
    
public:
    DeepSeekAPI(const std::string& api_key, const std::string& model = "deepseek-chat")
        : api_key_(api_key), model_(model), base_url_("https://api.deepseek.com") {}
    
    std::string chat(const std::vector<nlohmann::json>& messages) {
        CURL* curl = curl_easy_init();
        if (!curl) return "";
        
        nlohmann::json request;
        request["model"] = model_;
        request["messages"] = messages;
        request["stream"] = false;
        
        std::string url = base_url_ + "/chat/completions";
        std::string response;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.dump().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key_).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) return "";
        
        try {
            auto j = nlohmann::json::parse(response);
            if (j.contains("choices") && j["choices"].size() > 0) {
                return j["choices"][0]["message"]["content"].get<std::string>();
            }
        } catch (...) {}
        
        return "";
    }
    
    std::string chatStream(const std::vector<nlohmann::json>& messages, 
                           std::function<void(const std::string&)> onChunk) {
        CURL* curl = curl_easy_init();
        if (!curl) return "";
        
        nlohmann::json request;
        request["model"] = model_;
        request["messages"] = messages;
        request["stream"] = true;
        
        std::string url = base_url_ + "/chat/completions";
        StreamData data;
        data.callback = &onChunk;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.dump().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, streamCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key_).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: text/event-stream");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) return "";
        
        return "[DONE]";
    }
    
private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
};

#endif // API_HPP
