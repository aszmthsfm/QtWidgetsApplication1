#include "ConfigLoader.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

// 辅助函数：将 Hex 字符串 (#FF0000) 转为 QColor
QColor hexToColor(const QString& hex) {
    // 【修复】Qt6 推荐使用 isValidColorName
    if (QColor::isValidColorName(hex)) {
        return QColor(hex);
    }
    // 如果颜色无效，返回黑色作为兜底
    return Qt::black;
}

AppConfig ConfigLoader::load(const QString& configPath) {
    AppConfig config; // 先创建一个带有默认值的配置对象

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Config Warning: Could not open" << configPath << "- Using default values.";
        return config; // 打开失败，直接返回默认配置
    }

    QByteArray val = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(val);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "Config Error: JSON format invalid.";
        return config;
    }

    QJsonObject root = doc.object();

    // 1. 读取 Window 配置
    if (root.contains("window")) {
        QJsonObject winObj = root["window"].toObject();
        config.window.width = winObj["width"].toInt(config.window.width);
        config.window.height = winObj["height"].toInt(config.window.height);
        config.window.title = winObj["title"].toString(config.window.title);
    }

    // 2. 读取 Simulation 配置
    if (root.contains("simulation")) {
        QJsonObject simObj = root["simulation"].toObject();
        config.sim.targetFPS = simObj["targetFPS"].toInt(config.sim.targetFPS);
        config.sim.vehicleCount = simObj["vehicleCount"].toInt(config.sim.vehicleCount);
    }

    // 3. 读取 Vehicle 配置
    if (root.contains("vehicle")) {
        QJsonObject vehObj = root["vehicle"].toObject();
        config.vehicle.length = (float)vehObj["length"].toDouble(config.vehicle.length);
        config.vehicle.width = (float)vehObj["width"].toDouble(config.vehicle.width);
        config.vehicle.minSpeed = (float)vehObj["minSpeed"].toDouble(config.vehicle.minSpeed);
        config.vehicle.maxSpeed = (float)vehObj["maxSpeed"].toDouble(config.vehicle.maxSpeed);

        // 读取颜色数组
        if (vehObj.contains("colors") && vehObj["colors"].isArray()) {
            QJsonArray colorArray = vehObj["colors"].toArray();
            // 如果 JSON 里有定义颜色，才覆盖默认值，否则使用默认的彩虹色
            if (!colorArray.isEmpty()) {
                config.vehicle.colors.clear();
                for (const auto& val : colorArray) {
                    config.vehicle.colors.append(hexToColor(val.toString()));
                }
            }
        }
    }

    // 4. 读取 Map 配置
    if (root.contains("map")) {
        QJsonObject mapObj = root["map"].toObject();
        config.map.netFilePath = mapObj["netFilePath"].toString(config.map.netFilePath);
        config.map.backgroundColor = hexToColor(mapObj["backgroundColor"].toString(config.map.backgroundColor.name()));
        config.map.junctionColor = hexToColor(mapObj["junctionColor"].toString(config.map.junctionColor.name()));
        config.map.internalLaneColor = hexToColor(mapObj["internalLaneColor"].toString(config.map.internalLaneColor.name()));
        config.map.roadColor = hexToColor(mapObj["roadColor"].toString(config.map.roadColor.name()));

        config.map.internalLineWidth = (float)mapObj["internalLineWidth"].toDouble(config.map.internalLineWidth);
        config.map.roadLineWidth = (float)mapObj["roadLineWidth"].toDouble(config.map.roadLineWidth);
        config.map.defaultLocalZoom = (float)mapObj["defaultLocalZoom"].toDouble(config.map.defaultLocalZoom);
    }

    qDebug() << "Config loaded successfully from" << configPath;
    return config;
}