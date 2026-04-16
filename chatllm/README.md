# ChatLLM

终端 LLM 聊天客户端，支持 DeepSeek API。

## 功能

- 💬 多轮对话（记得上文）
- 📁 对话历史自动保存
- 🖼️ 支持图片输入（多模态）
- 📜 流式输出（打字效果）
- 📋 多会话管理

## 安装

### 依赖

- CMake >= 3.10
- libcurl
- nlohmann-json (header-only)

### 构建

```bash
# Ubuntu/Debian
sudo apt install libcurl-dev nlohmann-json3-dev cmake g++

# macOS
brew install curl nlohmann-json cmake

# 构建
mkdir build && cd build
cmake ..
make
```

## 配置

设置 DeepSeek API Key：

```bash
export DEEPSEEK_API_KEY=your_api_key_here
```

可选配置：
```bash
export DEEPSEEK_MODEL=deepseek-chat        # 模型，默认 deepseek-chat
export DEEPSEEK_BASE_URL=https://api.deepseek.com  # API 地址
```

## 使用

```bash
./chatllm
```

### 命令

| 命令 | 说明 |
|------|------|
| `/new [标题]` | 创建新会话 |
| `/list` | 查看所有会话 |
| `/switch <id>` | 切换会话 |
| `/delete <id>` | 删除会话 |
| `/clear` | 清屏 |
| `/quit` | 退出 |

直接输入文字即可对话，输入图片路径可发送图片。

## 项目结构

```
chatllm/
├── src/
│   └── main.cpp       # 主程序
├── include/
│   ├── config.hpp     # 配置
│   ├── session.hpp    # 会话管理
│   └── api.hpp        # API 调用
├── build/             # 构建目录
├── CMakeLists.txt
└── README.md
```

## License

MIT
