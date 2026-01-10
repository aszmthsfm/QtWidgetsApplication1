#pragma once
#include "Config.h"
#include <QString>

class ConfigLoader {
public:
    // 静态函数：传入文件路径，返回一个填好数据的 AppConfig 对象
    static AppConfig load(const QString& configPath);
};