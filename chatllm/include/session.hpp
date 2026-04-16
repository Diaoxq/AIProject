#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <ctime>

namespace fs = std::filesystem;

enum class MessageRole {
    User,
    Assistant,
    System
};

struct Message {
    MessageRole role;
    std::string content;
    std::string image_path;  // 如果是图片消息，存储图片路径
    
    Message(MessageRole r, const std::string& c, const std::string& img = "")
        : role(r), content(c), image_path(img) {}
    
    std::string roleToString() const {
        switch (role) {
            case MessageRole::User: return "user";
            case MessageRole::Assistant: return "assistant";
            case MessageRole::System: return "system";
            default: return "user";
        }
    }
    
    nlohmann::json toJson() const {
        nlohmann::json j;
        j["role"] = roleToString();
        
        if (!image_path.empty()) {
            // 多模态消息：同时包含文本和图片
            if (content.empty()) {
                j["content"] = {
                    {
                        {"type", "image_url"},
                        {"image_url", {{"url", "file://" + image_path}}}
                    }
                };
            } else {
                j["content"] = {
                    {{"type", "text"}, {"text", content}},
                    {
                        {"type", "image_url"},
                        {"image_url", {{"url", "file://" + image_path}}}
                    }
                };
            }
        } else {
            j["content"] = content;
        }
        return j;
    }
    
    static Message fromJson(const nlohmann::json& j) {
        MessageRole role;
        std::string role_str = j.value("role", "user");
        if (role_str == "user") role = MessageRole::User;
        else if (role_str == "assistant") role = MessageRole::Assistant;
        else role = MessageRole::System;
        
        std::string content;
        std::string image_path;
        
        if (j.contains("content")) {
            auto& content_val = j["content"];
            if (content_val.is_string()) {
                content = content_val.get<std::string>();
            } else if (content_val.is_array()) {
                // 处理多模态格式
                for (auto& item : content_val) {
                    if (item["type"] == "text") {
                        content = item.value("text", "");
                    } else if (item["type"] == "image_url") {
                        image_path = item["image_url"].value("url", "");
                        // 移除 file:// 前缀
                        if (image_path.find("file://") == 0) {
                            image_path = image_path.substr(7);
                        }
                    }
                }
            }
        }
        
        return Message(role, content, image_path);
    }
};

struct Session {
    int id;
    std::string title;
    std::string created_at;
    std::string updated_at;
    std::vector<Message> messages;
    
    Session(int i, const std::string& t) 
        : id(i), title(t) {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        created_at = updated_at = oss.str();
    }
    
    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["title"] = title;
        j["created_at"] = created_at;
        j["updated_at"] = updated_at;
        j["messages"] = nlohmann::json::array();
        for (auto& msg : messages) {
            j["messages"].push_back(msg.toJson());
        }
        return j;
    }
    
    static Session fromJson(const nlohmann::json& j) {
        Session s(j.value("id", 0), j.value("title", "会话"));
        s.created_at = j.value("created_at", "");
        s.updated_at = j.value("updated_at", "");
        if (j.contains("messages")) {
            for (auto& msg_j : j["messages"]) {
                s.messages.push_back(Message::fromJson(msg_j));
            }
        }
        return s;
    }
    
    void updateTime() {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        updated_at = oss.str();
    }
};

class SessionManager {
private:
    std::string data_dir_;
    int next_id_;
    Session* current_session_;
    std::vector<Session> sessions_;
    
    std::string getSessionFilePath(int id) const {
        return data_dir_ + "/sessions/" + std::to_string(id) + ".json";
    }
    
public:
    SessionManager(const std::string& data_dir) : data_dir_(data_dir), next_id_(1), current_session_(nullptr) {
        fs::create_directories(data_dir_ + "/sessions");
        loadSessionList();
    }
    
    ~SessionManager() {
        if (current_session_) {
            saveCurrentSession();
        }
    }
    
    void loadSessionList() {
        sessions_.clear();
        fs::path dir(data_dir_ + "/sessions");
        if (!fs::exists(dir)) return;
        
        for (auto& entry : fs::directory_iterator(dir)) {
            if (entry.path().extension() == ".json") {
                try {
                    std::ifstream f(entry.path());
                    auto j = nlohmann::json::parse(f);
                    sessions_.push_back(Session::fromJson(j));
                    next_id_ = std::max(next_id_, sessions_.back().id + 1);
                } catch (...) {
                    // 忽略无效文件
                }
            }
        }
        
        std::sort(sessions_.begin(), sessions_.end(), 
            [](const Session& a, const Session& b) {
                return a.updated_at > b.updated_at;
            });
    }
    
    Session& createSession(const std::string& title) {
        saveCurrentSession();
        
        Session s(next_id_++, title);
        sessions_.insert(sessions_.begin(), s);
        current_session_ = &sessions_.front();
        saveCurrentSession();
        return *current_session_;
    }
    
    void loadSession(int id) {
        saveCurrentSession();
        
        auto it = std::find_if(sessions_.begin(), sessions_.end(), 
            [id](const Session& s) { return s.id == id; });
        
        if (it != sessions_.end()) {
            current_session_ = &(*it);
            // 移动到最前面
            std::rotate(sessions_.begin(), it, std::next(it));
        }
    }
    
    void deleteSession(int id) {
        sessions_.erase(
            std::remove_if(sessions_.begin(), sessions_.end(),
                [id](const Session& s) { return s.id == id; }),
            sessions_.end()
        );
        
        fs::remove(getSessionFilePath(id));
        
        if (current_session_ && current_session_->id == id) {
            current_session_ = sessions_.empty() ? nullptr : &sessions_.front();
        }
    }
    
    Session& currentSession() {
        if (!current_session_) {
            createSession("默认会话");
        }
        return *current_session_;
    }
    
    std::vector<Session> listSessions() const {
        return sessions_;
    }
    
    void addMessage(MessageRole role, const std::string& content, bool is_image = false) {
        currentSession().messages.push_back(Message(role, content, is_image ? content : ""));
        currentSession().updateTime();
        saveCurrentSession();
    }
    
    void addMessage(MessageRole role, const std::string& content, const std::string& image_path) {
        currentSession().messages.push_back(Message(role, content, image_path));
        currentSession().updateTime();
        saveCurrentSession();
    }
    
    void appendToLastMessage(const std::string& content) {
        if (!currentSession().messages.empty()) {
            currentSession().messages.back().content += content;
        }
    }
    
    void finalizeLastMessage(const std::string& content) {
        if (!currentSession().messages.empty()) {
            currentSession().messages.back().content = content;
            currentSession().updateTime();
            saveCurrentSession();
        }
    }
    
    void removeLastMessage() {
        if (!currentSession().messages.empty()) {
            currentSession().messages.pop_back();
            saveCurrentSession();
        }
    }
    
    void saveCurrentSession() {
        if (current_session_) {
            std::ofstream f(getSessionFilePath(current_session_->id));
            f << current_session_->toJson().dump(2);
        }
    }
};

#endif // SESSION_HPP
