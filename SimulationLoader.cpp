#include "SimulationLoader.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QtGlobal>

SimulationLoader::SimulationLoader() {
}

bool SimulationLoader::init(const QString& directoryPath) {
    m_dataDir = directoryPath;
    QDir dir(m_dataDir);
    QStringList filters;
    filters << "*.json";
    dir.setNameFilters(filters);

    // 按名称排序确保时间顺序
    m_jsonFiles = dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    if (m_jsonFiles.isEmpty()) {
        qDebug() << "SimulationLoader: No JSON files found in" << directoryPath;
        return false;
    }
    qDebug() << "SimulationLoader: Found" << m_jsonFiles.size() << "frames.";
    return true;
}

QVector<Vehicle> SimulationLoader::loadFrame(int frameIndex) {
    QVector<Vehicle> rawVehicles;

    if (m_jsonFiles.isEmpty()) return rawVehicles;

    // 循环播放逻辑：如果越界，取模或归零
    int safeIndex = frameIndex % m_jsonFiles.size();
    QString fileName = m_jsonFiles[safeIndex];
    QString fullPath = m_dataDir + "/" + fileName;

    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error reading frame:" << fullPath;
        return rawVehicles;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isArray()) {
        return rawVehicles;
    }

    QJsonArray vehArray = doc.array();
    for (const auto& val : vehArray) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        Vehicle v;
        v.id = obj["id"].toString();
        v.x = (float)obj["position_x"].toDouble();
        v.y = (float)obj["position_y"].toDouble();
        v.angle = (float)obj["heading"].toDouble();
        v.length = (float)obj["length"].toDouble(4.5);
        v.width = (float)obj["width"].toDouble(1.8);
        v.speed = (float)obj["velocity"].toDouble();
        v.currentEdgeId = obj["road_id"].toString();
        v.currentLaneIndex = obj["lane_index"].toInt(0);
        v.missingFrames = 0;

        // 颜色生成 (保持一致性)
        uint hash = qHash(v.id);
        int hue = hash % 360;
        v.color = QColor::fromHsv(hue, 200, 240);

        rawVehicles.append(v);
    }

    return rawVehicles;
}