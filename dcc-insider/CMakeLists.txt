# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0
cmake_minimum_required(VERSION 3.7)

set(PLUGIN_NAME "dcc-insider-plugin")

project(${PLUGIN_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "-g -Wall")

# 增加安全编译参数
ADD_DEFINITIONS("-fstack-protector-strong -D_FORTITY_SOURCE=1 -z noexecstack -pie -fPIC -z lazy")

find_package(Qt6 COMPONENTS Core Gui DBus LinguistTools REQUIRED)
find_package(DdeControlCenter REQUIRED)
find_package(PkgConfig REQUIRED)


set(DccInsider_Name insider)
file(GLOB_RECURSE DccInsider_SRCS
    "*.cpp"
    "*.h"
)
add_library(${DccInsider_Name} MODULE
    ${DccInsider_SRCS}
)

target_include_directories(${DccInsider_Name} PUBLIC
    Dde::Control-Center
)

set(DccInsider_Libraries
    Qt6::DBus
    Qt6::Gui
    Dde::Control-Center
)

target_link_libraries(${DccInsider_Name} PRIVATE
    ${DccInsider_Libraries}
)

dcc_install_plugin(NAME ${DccInsider_Name} TARGET ${DccInsider_Name})
dcc_handle_plugin_translation(NAME ${DccInsider_Name})
