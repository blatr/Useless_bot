#pragma once

#include <memory>
#include <vector>
#include <fstream>
#include <iostream>
#include <random>
#include <optional>

typedef std::vector<std::pair<std::string, std::string>> JsonPairs;

struct GetMeResponseType {
    std::string username;
    bool is_bot;
    std::string first_name;
    int64_t id;
};

struct TelegramAPIError : public std::runtime_error {
    TelegramAPIError(int http_code_p, const std::string& details_p);
    int http_code;
    std::string details;
};

struct MessageResponseType {
    std::string message_text;
    std::string chat_type;
    bool group_chat_created;
    int64_t chat_id;
    int64_t reply_to_message_id;
};

struct UpdateResponseType {
    UpdateResponseType(const int64_t update_id_, const MessageResponseType& message_);

    int64_t update_id;
    MessageResponseType message;
};

class TgBotClient {
public:
    TgBotClient(const std::string& api_token, const std::string& end_point);

    std::string CreateRequestString(const std::string& handle);

    std::vector<UpdateResponseType> GetUpdates(int64_t offset = 0, int64_t timeout = 0);

    GetMeResponseType GetMe();

    MessageResponseType SendMessage(
        int64_t chat_id, const std::string& text,
        const std::optional<int64_t>& reply_to_message_id = std::nullopt);

    void SendMedia(int64_t chat_id, const std::string& media_id, std::string type);

private:
    std::string api_token_;
    std::string end_point_;
};

class IBot {
public:
    IBot(const std::string& api_token, const std::string& end_point);

    IBot(std::unique_ptr<TgBotClient>& client);

    virtual ~IBot();
    void Run();

    void SaveOffset(int64_t offset);

    int64_t RestoreOffsetFromFile();

    virtual bool onMsg(const UpdateResponseType& update) = 0;

protected:
    std::unique_ptr<TgBotClient> client_;
};

class MyBot : public IBot {
public:
    MyBot(std::unique_ptr<TgBotClient>& client);
    MyBot(const std::string& api_token, const std::string& end_point);
    bool onMsg(const UpdateResponseType& update) override;

private:
    std::mt19937 generator;
};