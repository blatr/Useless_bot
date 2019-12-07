#include "bot.h"
#include <Poco/URI.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/JSON/JSON.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>

TelegramAPIError::TelegramAPIError(int http_code_p, const std::string& details_p)
    : std::runtime_error("api error: code=" + std::to_string(http_code_p) +
                         " details=" + details_p),
      http_code(http_code_p),
      details(details_p) {
}
UpdateResponseType::UpdateResponseType(const int64_t update_id_,
                                       const MessageResponseType& message_)
    : update_id(update_id_), message(message_) {
}
TgBotClient::TgBotClient(const std::string& api_token, const std::string& end_point)
    : api_token_(api_token), end_point_(end_point) {
}
std::string TgBotClient::CreateRequestString(const std::string& handle) {
    return end_point_ + "bot" + api_token_ + handle;
}
std::vector<UpdateResponseType> TgBotClient::GetUpdates(int64_t offset, int64_t timeout) {
    auto request_string = CreateRequestString("/getUpdates");
    Poco::URI uri(request_string);
    if (offset != 0) {
        uri.addQueryParameter("offset", std::to_string(offset));
    }
    if (timeout != 0) {
        uri.addQueryParameter("timeout", std::to_string(timeout));
    }

    Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
    Poco::Net::HTTPRequest request("GET", uri.getPathEtc());
    session.sendRequest(request);

    Poco::Net::HTTPResponse response;
    auto& body = session.receiveResponse(response);
    if (response.getStatus() != 200) {
        throw TelegramAPIError(response.getStatus(), "server error");
    }
    Poco::JSON::Parser parser;
    auto reply = parser.parse(body);

    std::vector<UpdateResponseType> updates;

    auto update_array = reply.extract<Poco::JSON::Object::Ptr>()->getArray("result");

    for (int i = 0; i < update_array->size(); ++i) {
        MessageResponseType msg;
        auto result_obj = update_array->getObject(i);
        auto msg_obj = result_obj->getObject("message");
        auto msg_chat_obj = msg_obj->getObject("chat");
        auto msg_reply_obj = msg_obj->getObject("reply_to_message");
        std::string filler = "";
        msg.message_text = msg_obj->optValue("text", filler);
        msg.chat_type = msg_chat_obj->optValue("type", filler);
        msg.group_chat_created = msg_obj->optValue("group_chat_created", false);
        msg.chat_id = msg_chat_obj->getValue<int64_t>("id");
        if (msg_reply_obj) {
            msg.reply_to_message_id = msg_reply_obj->getValue<int64_t>("message_id");
        }
        updates.emplace_back(result_obj->getValue<int64_t>("update_id"), msg);
    }
    return updates;
}
GetMeResponseType TgBotClient::GetMe() {
    auto request_string = CreateRequestString("/getMe");
    Poco::URI uri(request_string);
    Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
    Poco::Net::HTTPRequest request("GET", uri.getPathEtc());
    session.sendRequest(request);

    Poco::Net::HTTPResponse response;
    auto& body = session.receiveResponse(response);
    if (response.getStatus() != 200) {
        throw TelegramAPIError(response.getStatus(), "server error");
    }
    Poco::JSON::Parser parser;
    auto reply = parser.parse(body);

    auto json_body = reply.extract<Poco::JSON::Object::Ptr>();
    GetMeResponseType getme;
    auto ok = json_body->getValue<bool>("ok");
    if (ok) {
        auto me_obj = reply.extract<Poco::JSON::Object::Ptr>()->getObject("result");
        std::string filler = "";
        getme.first_name = me_obj->optValue("first_name", filler);
        getme.id = me_obj->optValue("id", 0);
        getme.is_bot = me_obj->optValue("is_bot", false);
        getme.username = me_obj->optValue("username", filler);
    }
    return getme;
}
MessageResponseType TgBotClient::SendMessage(int64_t chat_id, const std::string& text,
                                             const std::optional<int64_t>& reply_to_message_id) {

    auto request_string = CreateRequestString("/sendMessage");
    Poco::URI uri(request_string);
    Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());

    JsonPairs headers;
    headers.emplace_back("Content-Type", "application/json");

    Poco::JSON::Object obj;
    obj.set("chat_id", chat_id);
    obj.set("text", text);
    if (reply_to_message_id.has_value()) {
        obj.set("reply_to_message_id", reply_to_message_id.value());
    }

    Poco::Net::HTTPRequest request("POST", uri.getPathEtc());
    for (const auto& [header, value] : headers) {
        request.add(header, value);
    }
    std::stringstream ss;
    obj.stringify(ss);
    request.setContentLength(ss.str().size());
    obj.stringify(session.sendRequest(request));

    Poco::Net::HTTPResponse response;
    auto& body = session.receiveResponse(response);
    if (response.getStatus() != 200) {
        throw TelegramAPIError(response.getStatus(), "server error");
    }

    Poco::JSON::Parser parser;
    auto reply = parser.parse(body);

    MessageResponseType msg;
    auto msg_obj = reply.extract<Poco::JSON::Object::Ptr>()->getObject("result");
    auto msg_chat_obj = msg_obj->getObject("chat");
    auto msg_reply_obj = msg_obj->getObject("reply_to_message");
    std::string filler = "";
    msg.message_text = msg_obj->optValue("text", filler);
    msg.chat_type = msg_chat_obj->optValue("type", filler);
    msg.group_chat_created = msg_obj->optValue("group_chat_created", false);
    msg.chat_id = msg_chat_obj->getValue<int64_t>("id");
    if (msg_reply_obj) {
        msg.reply_to_message_id = msg_reply_obj->getValue<int64_t>("message_id");
    }

    return msg;
}
void TgBotClient::SendMedia(int64_t chat_id, const std::string& media_id, std::string type) {
    auto request_string = CreateRequestString("/send" + type);
    type[0] = tolower(type[0]);
    Poco::URI uri(request_string);
    Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());

    JsonPairs headers;
    headers.emplace_back("Content-Type", "application/json");

    Poco::JSON::Object obj;
    obj.set("chat_id", chat_id);
    obj.set(type, media_id);
    Poco::Net::HTTPRequest request("POST", uri.getPathEtc());
    for (const auto& [header, value] : headers) {
        request.add(header, value);
    }
    std::stringstream ss;
    obj.stringify(ss);
    request.setContentLength(ss.str().size());
    obj.stringify(session.sendRequest(request));

    Poco::Net::HTTPResponse response;
    auto& body = session.receiveResponse(response);
    if (response.getStatus() != 200) {
        throw TelegramAPIError(response.getStatus(), "server error");
    }
}
IBot::IBot(const std::string& api_token, const std::string& end_point)
    : client_(std::make_unique<TgBotClient>(api_token, end_point)) {
}

IBot::IBot(std::unique_ptr<TgBotClient>& client) : client_(std::move(client)) {
}
IBot::~IBot() = default;
void IBot::Run() {
    try {
        bool run_bot = true;
        while (run_bot) {
            auto offset = RestoreOffsetFromFile();
            auto updates = client_->GetUpdates(offset);
            for (const auto& upd : updates) {
                run_bot = onMsg(upd);
            }
        }
    } catch (const TelegramAPIError& e) {
        std::cout << "Error: " << e.what();
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what();
    }
}
void IBot::SaveOffset(int64_t offset) {
    std::ofstream offset_file;
    offset_file.open("offset.txt");
    offset_file << offset << "\n";
    offset_file.close();
}
int64_t IBot::RestoreOffsetFromFile() {
    std::string line;
    std::ifstream offset_file("offset.txt");
    if (offset_file.is_open()) {
        getline(offset_file, line);
        offset_file.close();
    }
    if (line.empty()) {
        return 0;
    }
    return std::stoi(line);
};

MyBot::MyBot(std::unique_ptr<TgBotClient>& client) : IBot(client) {
}
MyBot::MyBot(const std::string& api_token, const std::string& end_point)
    : IBot(api_token, end_point), generator(37) {
}
bool MyBot::onMsg(const UpdateResponseType& update) {
    if (update.message.message_text == "/random") {
        std::uniform_int_distribution dist(0, 1000);
        client_->SendMessage(update.message.chat_id, std::to_string(dist(generator)));
        SaveOffset(update.update_id + 1);
    } else if (update.message.message_text == "/weather") {
        client_->SendMessage(update.message.chat_id, "Winter Is Coming");
        SaveOffset(update.update_id + 1);
    } else if (update.message.message_text == "/styleguide") {
        client_->SendMessage(update.message.chat_id, "Code review is a joke");
        client_->SendMedia(update.message.chat_id, "CgADBAADs6kAAsMXZAd1OLcYWalBdhYE", "animation");
        SaveOffset(update.update_id + 1);
    } else if (update.message.message_text == "/sticker") {
        client_->SendMedia(update.message.chat_id, "CAADAgADLQcAAlwCZQMbhEfcuESw4RYE", "Sticker");
        SaveOffset(update.update_id + 1);
    } else if (update.message.message_text == "/gif") {
        client_->SendMedia(update.message.chat_id, "CgADBAADs6kAAsMXZAd1OLcYWalBdhYE", "Animation");
        SaveOffset(update.update_id + 1);
    } else if (update.message.message_text == "/stop") {
        SaveOffset(update.update_id + 1);
        return false;
    } else if (update.message.message_text == "/crash") {
        SaveOffset(update.update_id + 1);
        abort();
    }
    return true;
}
