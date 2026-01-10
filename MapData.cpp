#include "MapData.h"
#include <QDomDocument>
#include <QFile>
#include <QDebug>
#include <QtMath>
#include <QLineF> 
#include <cmath>
#include <QRandomGenerator>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

MapData::MapData() {
    m_bounds = QRectF(0, 0, 100, 100);
}

// 【新增】初始化回放目录，获取所有JSON文件并排序
void MapData::initPlayback(const QString& directoryPath) {
    m_dataDir = directoryPath;
    QDir dir(m_dataDir);

    // 设置过滤器，只读取json文件
    QStringList filters;
    filters << "*.json";
    dir.setNameFilters(filters);

    // 获取文件列表并按名称排序（时间戳命名通常按名称排序即按时间排序）
    m_jsonFiles = dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    m_currentFrameIndex = 0;

    if (m_jsonFiles.isEmpty()) {
        qDebug() << "Warning: No JSON files found in" << directoryPath;
    }
    else {
        qDebug() << "Playback initialized. Found" << m_jsonFiles.size() << "frames in" << directoryPath;
    }
}

void MapData::parseShape(const QString& shapeStr, QVector<QPointF>& outPoints) {
    QStringList points = shapeStr.split(' ', Qt::SkipEmptyParts);
    for (const QString& ptStr : points) {
        QStringList coords = ptStr.split(',');
        if (coords.size() == 2) {
            outPoints.append(QPointF(coords[0].toFloat(), coords[1].toFloat()));
        }
    }
}

bool MapData::load(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QDomDocument doc;
    if (!doc.setContent(&file)) { file.close(); return false; }
    file.close();

    m_edges.clear(); m_junctions.clear(); m_vehicles.clear();
    QDomElement root = doc.documentElement();
    QDomNode node = root.firstChild();
    float minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    bool hasData = false;

    while (!node.isNull()) {
        QDomElement element = node.toElement();
        if (!element.isNull() && element.tagName() == "edge") {
            Edge edge;
            edge.id = element.attribute("id");
            edge.fromJunc = element.attribute("from");
            edge.toJunc = element.attribute("to");
            edge.function = element.attribute("function");
            QDomNode childNode = element.firstChild();
            while (!childNode.isNull()) {
                QDomElement childElem = childNode.toElement();
                if (childElem.tagName() == "lane") {
                    Lane lane;
                    lane.id = childElem.attribute("id");
                    lane.width = childElem.attribute("width", "3.0").toFloat();
                    parseShape(childElem.attribute("shape"), lane.shape);
                    for (const auto& pt : lane.shape) {
                        minX = qMin((float)pt.x(), minX); maxX = qMax((float)pt.x(), maxX);
                        minY = qMin((float)pt.y(), minY); maxY = qMax((float)pt.y(), maxY);
                        hasData = true;
                    }
                    edge.lanes.append(lane);
                }
                childNode = childNode.nextSibling();
            }
            m_edges.append(edge);
        }
        else if (!element.isNull() && element.tagName() == "junction" && element.attribute("type") != "internal") {
            Junction junc;
            junc.id = element.attribute("id");
            parseShape(element.attribute("shape"), junc.shape);
            m_junctions.append(junc);
        }
        node = node.nextSibling();
    }
    if (hasData) m_bounds = QRectF(minX, minY, maxX - minX, maxY - minY);
    return true;
}

// 占位函数，回放模式下不再使用
void MapData::spawnVehicle() {
}

void MapData::updateSimulationStep() {
    // 1. 基本校验
    if (m_jsonFiles.isEmpty()) return;
    if (m_currentFrameIndex >= m_jsonFiles.size()) m_currentFrameIndex = 0;

    QString fileName = m_jsonFiles[m_currentFrameIndex];
    QString fullPath = m_dataDir + "/" + fileName;

    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_currentFrameIndex++;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isArray()) {
        m_currentFrameIndex++;
        return;
    }

    // =========== 【修改步骤 1】保存上一帧的车辆列表 ===========
    QMap<QString, Vehicle> prevVehiclesMap;
    for (const auto& v : m_vehicles) {
        prevVehiclesMap.insert(v.id, v);
    }
    // ======================================================

    // 清空当前车辆列表
    m_vehicles.clear();

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

        // 颜色生成逻辑
        uint hash = qHash(v.id);
        int hue = hash % 360;
        v.color = QColor::fromHsv(hue, 200, 240);

        v.currentEdgeId = obj["road_id"].toString();
        v.currentLaneIndex = obj["lane_index"].toInt(0);

        m_vehicles.append(v);
    }

    // =========== 【修改步骤 2】智能排查逻辑 ===========

    // 1. 找出这一帧里消失的车辆
    for (auto it = prevVehiclesMap.begin(); it != prevVehiclesMap.end(); ++it) {
        QString oldId = it.key();
        Vehicle oldVeh = it.value();

        // 检查这个 ID 是否还在新的一帧里
        bool stillExists = false;
        for (const auto& newVeh : m_vehicles) {
            if (newVeh.id == oldId) {
                stillExists = true;
                break;
            }
        }

        if (!stillExists) {
            // 这辆车消失了！判断是正常离开还是异常消失
            // 判断逻辑：如果离地图边缘小于 10 米，算正常离开
            float distLeft = abs(oldVeh.x - m_bounds.left());
            float distRight = abs(oldVeh.x - m_bounds.right());
            float distTop = abs(oldVeh.y - m_bounds.top());
            float distBottom = abs(oldVeh.y - m_bounds.bottom());

            float minEdgeDist = std::min({ distLeft, distRight, distTop, distBottom });

            if (minEdgeDist < 10.0f) {
                // 离边缘很近，认为是正常离开，不打印 Log 以免刷屏
                // qDebug() << "Vehicle" << oldId << "left map normally."; 
            }
            else {
                // 离边缘很远就没了？这就是 BUG！
                qDebug() << "!!! ABNORMAL DISAPPEARANCE:" << oldId
                    << "at (" << oldVeh.x << "," << oldVeh.y << ")"
                    << "Dist to edge:" << minEdgeDist
                    << "Frame:" << m_currentFrameIndex;
            }
        }
    }

    // 2. (可选) 找出这一帧里凭空出现的车辆
    /*
    for (const auto& newVeh : m_vehicles) {
        if (!prevVehiclesMap.contains(newVeh.id)) {
            // 这是一辆新车
            float distLeft = abs(newVeh.x - m_bounds.left());
            // ... 同上计算 minEdgeDist ...
            // if (minEdgeDist > 10.0f) qDebug() << "!!! ABNORMAL APPEARANCE:" << newVeh.id;
        }
    }
    */
    // ======================================================

    m_currentFrameIndex++;
    m_stepCounter++;
}

QPointF MapData::getJunctionPosition(const QString& id) {
    for (const auto& junc : m_junctions) {
        if (junc.id == id) {
            float sX = 0, sY = 0;
            if (junc.shape.isEmpty()) return QPointF(0, 0);
            for (const auto& pt : junc.shape) { sX += pt.x(); sY += pt.y(); }
            return QPointF(sX / (float)junc.shape.size(), sY / (float)junc.shape.size());
        }
    }
    return QPointF(0, 0);
}