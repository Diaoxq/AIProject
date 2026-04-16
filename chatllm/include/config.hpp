#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>

class Config {
public:
    std::string api_key;
    std::string model;
    std::string data_dir;
    std::string base_url;
    
    Config() {
        model = "deepseek-chat";
        data_dir = "./data";
        base_url = "https://api.deepseek.com";
    }
    
    bool load() {
        // 从环境变量加载
        char* key = std::getenv("DEEPSEEK_API_KEY");
        if (key) {
            api_key = key;
        }
        
        char* model_env = std::getenv("DEEPSEEK_MODEL");
        if (model_env) {
            model = model_env;
        }
        
        char* base_url_env = std::getenv("DEEPSEEK_BASE_URL");
        if (base_url_env) {
            base_url = base_url_env;
        }
        
        return !api_key.empty();
    }
    
    bool save() {
        // 可扩展：保存配置到文件
        return true;
    }
};

#endif // CONFIG_HPP
