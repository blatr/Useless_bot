cmake_minimum_required(VERSION 3.14)
project(telegram_bott)

set(CMAKE_CXX_STANDARD 17)

add_library(tgbot
        bot.cpp)

target_link_libraries(tgbot
        PocoNet
        PocoNetSSL
        PocoFoundation
        PocoJSON)

add_executable(bot main.cpp)

target_link_libraries(bot
        tgbot)



