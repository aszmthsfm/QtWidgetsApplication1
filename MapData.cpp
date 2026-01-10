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

    // ==========================================
    //  核心修改：惯性导航算法 (Dead Reckoning)
    // ==========================================

    // 1. 读取当前帧的所有车辆数据到临时 Map 中
    QMap<QString, Vehicle> currentFrameVehicles;
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
        v.missingFrames = 0; // 新读到的数据，当然没有失联

        // 颜色生成 (保持颜色一致)
        uint hash = qHash(v.id);
        int hue = hash % 360;
        v.color = QColor::fromHsv(hue, 200, 240);

        currentFrameVehicles.insert(v.id, v);
    }

    // 2. 遍历上一帧的车辆列表 (m_vehicles)，决定去留
    QVector<Vehicle> nextVehicles;

    for (const auto& oldVeh : m_vehicles) {
        // 情况 A: 这辆车在当前帧数据里存在 -> 更新它
        if (currentFrameVehicles.contains(oldVeh.id)) {
            nextVehicles.append(currentFrameVehicles.value(oldVeh.id));
            currentFrameVehicles.remove(oldVeh.id); // 移除已处理的
        }
        // 情况 B: 这辆车不见了！ -> 判断是正常离开还是数据缺失
        else {
            // 计算离边缘的距离
            float distLeft = abs(oldVeh.x - m_bounds.left());
            float distRight = abs(oldVeh.x - m_bounds.right());
            float distTop = abs(oldVeh.y - m_bounds.top());
            float distBottom = abs(oldVeh.y - m_bounds.bottom());
            float minEdgeDist = std::min({ distLeft, distRight, distTop, distBottom });

            // 阈值设为 10 米：如果在边缘 10 米内消失，认为是正常离开
            if (minEdgeDist < 10.0f) {
                // 正常离开，不加入 nextVehicles，这辆车会被自动删除
            }
            else {
                // 异常消失 (在地图中间) -> 启动“幽灵模式”
                Vehicle ghostVeh = oldVeh;
                ghostVeh.missingFrames++;

                // 如果失联还没超过 15 帧 (约3秒)，我们帮它“脑补”位置
                if (ghostVeh.missingFrames < 15) {
                    // 简单的惯性预测：根据角度和速度计算位移
                    // SUMO 角度：0是北(+Y)，90是东(+X)，顺时针
                    // 转换成弧度
                    float rad = ghostVeh.angle * 3.1415926f / 180.0f;

                    // 注意：sin/cos 的对应关系取决于坐标系，这里按标准 SUMO/GPS 坐标系推算
                    float dx = ghostVeh.speed * sin(rad);
                    float dy = ghostVeh.speed * cos(rad);

                    // 如果你的坐标系是标准的数学坐标系，可能需要微调 sin/cos
                    ghostVeh.x += dx;
                    ghostVeh.y += dy;

                    // (可选) 可以在这里把 ghostVeh 的颜色变半透明，表示它是预测的
                    // ghostVeh.color.setAlpha(150); 

                    nextVehicles.append(ghostVeh);
                }
                // 如果失联太久，那还是删了吧
            }
        }
    }

    // 3. 把新出现的车（currentFrameVehicles 里剩下的）加进来
    for (auto it = currentFrameVehicles.begin(); it != currentFrameVehicles.end(); ++it) {
        nextVehicles.append(it.value());
    }

    // 4. 更新主列表
    m_vehicles = nextVehicles;

    // ==========================================

    m_currentFrameIndex++;
    m_stepCounter++;
}

void MapData::reset() {
    m_currentFrameIndex = 0;
    m_stepCounter = 0;
    m_vehicles.clear(); // 清空当前车辆，防止画面残留
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