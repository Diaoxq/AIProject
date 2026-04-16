#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "config.hpp"
#include "session.hpp"
#include "api.hpp"

void printWelcome() {
    std::cout << "\n";
    std::cout << "  ╔═══════════════════════════════════════╗\n";
    std::cout << "  ║         ChatLLM - 终端LLM客户端       ║\n";
    std::cout << "  ║                                       ║\n";
    std::cout << "  ║  /new  - 开始新会话                   ║\n";
    std::cout << "  ║  /list - 查看所有会话                 ║\n";
    std::cout << "  ║  /switch <id> - 切换会话              ║\n";
    std::cout << "  ║  /delete <id> - 删除会话              ║\n";
    std::cout << "  ║  /clear - 清屏                       ║\n";
    std::cout << "  ║  /quit - 退出程序                     ║\n";
    std::cout << "  ║  /help - 显示帮助                    ║\n";
    std::cout << "  ╚═══════════════════════════════════════╝\n";
    std::cout << "\n";
}

void printHelp() {
    std::cout << "\n=== ChatLLM 帮助 ===\n";
    std::cout << "命令:\n";
    std::cout << "  /new [标题]    - 创建新会话，可选标题\n";
    std::cout << "  /list          - 列出所有会话\n";
    std::cout << "  /switch <id>   - 切换到指定会话\n";
    std::cout << "  /delete <id>   - 删除指定会话\n";
    std::cout << "  /clear         - 清屏\n";
    std::cout << "  /quit          - 退出程序\n";
    std::cout << "  /help          - 显示帮助\n";
    std::cout << "\n使用说明:\n";
    std::cout << "  - 直接输入文字与LLM对话\n";
    std::cout << "  - 支持发送图片：输入图片路径即可\n";
    std::cout << "  - 对话内容自动保存\n";
    std::cout << "====================\n\n";
}

int main(int argc, char* argv[]) {
    Config config;
    
    // 加载配置
    if (!config.load()) {
        std::cerr << "错误: 配置文件加载失败\n";
        std::cerr << "请设置 DEEPSEEK_API_KEY 环境变量或创建配置文件\n";
        return 1;
    }
    
    // 检查API Key
    if (config.api_key.empty()) {
        std::cerr << "错误: 未设置 DEEPSEEK_API_KEY\n";
        std::cerr << "请设置环境变量: export DEEPSEEK_API_KEY=your_key\n";
        return 1;
    }
    
    SessionManager session_manager(config.data_dir);
    DeepSeekAPI api(config.api_key, config.model);
    
    printWelcome();
    
    // 默认创建或加载最新会话
    auto sessions = session_manager.listSessions();
    if (sessions.empty()) {
        session_manager.createSession("默认会话");
    } else {
        session_manager.loadSession(sessions.front().id);
    }
    
    std::string input;
    while (true) {
        std::cout << "\n[You] ";
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        // 处理命令
        if (input[0] == '/') {
            std::string cmd = input.substr(1);
            std::string args;
            
            size_t space_pos = cmd.find(' ');
            if (space_pos != std::string::npos) {
                args = cmd.substr(space_pos + 1);
                cmd = cmd.substr(0, space_pos);
            }
            
            if (cmd == "quit" || cmd == "q") {
                std::cout << "再见！\n";
                break;
            } else if (cmd == "help" || cmd == "h") {
                printHelp();
                continue;
            } else if (cmd == "new") {
                std::string title = args.empty() ? "新会话" : args;
                session_manager.createSession(title);
                std::cout << "已创建新会话: " << title << "\n";
                continue;
            } else if (cmd == "list" || cmd == "ls") {
                auto list = session_manager.listSessions();
                std::cout << "\n=== 会话列表 ===\n";
                for (auto& s : list) {
                    std::cout << "  [" << s.id << "] " << s.title;
                    if (s.id == session_manager.currentSession().id) {
                        std::cout << " (当前)";
                    }
                    std::cout << "\n";
                }
                std::cout << "================\n";
                continue;
            } else if (cmd == "switch" || cmd == "sw") {
                if (args.empty()) {
                    std::cout << "请指定会话ID: /switch <id>\n";
                    continue;
                }
                try {
                    int id = std::stoi(args);
                    session_manager.loadSession(id);
                    std::cout << "已切换到会话: " << session_manager.currentSession().title << "\n";
                } catch (...) {
                    std::cout << "无效的会话ID\n";
                }
                continue;
            } else if (cmd == "delete" || cmd == "del") {
                if (args.empty()) {
                    std::cout << "请指定会话ID: /delete <id>\n";
                    continue;
                }
                try {
                    int id = std::stoi(args);
                    session_manager.deleteSession(id);
                    std::cout << "已删除会话\n";
                } catch (...) {
                    std::cout << "无效的会话ID\n";
                }
                continue;
            } else if (cmd == "clear" || cmd == "cls") {
                #ifdef _WIN32
                    system("cls");
                #else
                    system("clear");
                #endif
                printWelcome();
                continue;
            } else {
                std::cout << "未知命令: /" << cmd << "\n";
                std::cout << "输入 /help 查看帮助\n";
                continue;
            }
        }
        
        // 检查是否为图片路径
        bool is_image = (input.find('.') != std::string::npos && 
                        (input.ends_with(".png") || input.ends_with(".jpg") || 
                         input.ends_with(".jpeg") || input.ends_with(".gif") ||
                         input.ends_with(".webp")));
        
        if (is_image) {
            // 添加图片消息
            session_manager.addMessage(MessageRole::User, "", input, true);
        } else {
            // 添加文本消息
            session_manager.addMessage(MessageRole::User, input);
        }
        
        // 发送请求
        std::cout << "\n[ChatLLM] ";
        std::cout.flush();
        
        std::string response = api.chatStream(
            session_manager.currentSession().messages,
            [&session_manager](const std::string& chunk) {
                std::cout << chunk << std::flush;
                session_manager.appendToLastMessage(chunk);
            }
        );
        
        std::cout << "\n";
        
        if (response.empty()) {
            std::cerr << "错误: API调用失败\n";
            session_manager.removeLastMessage();
        } else {
            session_manager.finalizeLastMessage(response);
        }
    }
    
    return 0;
}
