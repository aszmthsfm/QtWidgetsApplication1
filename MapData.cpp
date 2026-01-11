#include "MapData.h"
#include <QDebug>
#include <QtMath>
#include <cmath>
#include <algorithm>
#include <QMap>

MapData::MapData() {
    m_bounds = QRectF(0, 0, 100, 100);
}

void MapData::clear() {
    m_edges.clear();
    m_junctions.clear();
    m_vehicles.clear();
    m_bounds = QRectF(0, 0, 100, 100);
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

void MapData::initPlayback(const QString& directoryPath) {
    // 委托给 Loader
    bool success = m_simLoader.init(directoryPath);
    m_currentFrameIndex = 0;

    if (!success) {
        qDebug() << "MapData: Failed to init playback from" << directoryPath;
    }
}

void MapData::reset() {
    m_currentFrameIndex = 0;
    m_vehicles.clear();
}

void MapData::updateSimulationStep() {
    if (m_simLoader.frameCount() == 0) return;

    // 1. 从 Loader 获取当前帧的原始数据
    QVector<Vehicle> rawVehicles = m_simLoader.loadFrame(m_currentFrameIndex);

    // 2. 将原始数据转为 Map 以便快速查找
    QMap<QString, Vehicle> currentFrameMap;
    for (const auto& v : rawVehicles) {
        currentFrameMap.insert(v.id, v);
    }

    // 3. 执行惯性导航与平滑逻辑 (Core Simulation Logic)
    QVector<Vehicle> nextVehicles;

    // 遍历上一帧的车辆 (m_vehicles 是当前 MapData 维护的状态)
    for (const auto& oldVeh : m_vehicles) {
        // A. 如果新的一帧里还有这辆车 -> 直接更新状态
        if (currentFrameMap.contains(oldVeh.id)) {
            nextVehicles.append(currentFrameMap.value(oldVeh.id));
            currentFrameMap.remove(oldVeh.id); // 标记为已处理
        }
        // B. 如果车不见了 -> 尝试惯性预测 (Ghost Mode)
        else {
            float distLeft = std::abs(oldVeh.x - m_bounds.left());
            float distRight = std::abs(oldVeh.x - m_bounds.right());
            float distTop = std::abs(oldVeh.y - m_bounds.top());
            float distBottom = std::abs(oldVeh.y - m_bounds.bottom());
            float minEdgeDist = std::min({ distLeft, distRight, distTop, distBottom });

            // 只有在地图中间消失的才进行预测 (防止边缘离开的车被强行留住)
            if (minEdgeDist >= 10.0f) {
                Vehicle ghostVeh = oldVeh;
                ghostVeh.missingFrames++;

                // 最多预测 15 帧 (约3秒)
                if (ghostVeh.missingFrames < 15) {
                    float rad = ghostVeh.angle * 3.1415926f / 180.0f;
                    // 简单的匀速直线运动预测
                    ghostVeh.x += ghostVeh.speed * std::sin(rad);
                    ghostVeh.y += ghostVeh.speed * std::cos(rad);

                    nextVehicles.append(ghostVeh);
                }
            }
        }
    }

    // 4. 添加新出现的车辆 (Map 中剩余的)
    for (auto it = currentFrameMap.begin(); it != currentFrameMap.end(); ++it) {
        nextVehicles.append(it.value());
    }

    // 5. 更新状态
    m_vehicles = nextVehicles;

    // 6. 推进帧索引
    m_currentFrameIndex++;
    if (m_currentFrameIndex >= m_simLoader.frameCount()) {
        m_currentFrameIndex = 0; // 循环播放
    }
}

const Vehicle* MapData::getVehicle(const QString& id) const {
    if (id.isEmpty()) return nullptr;
    for (const auto& v : m_vehicles) {
        if (v.id == id) return &v;
    }
    return nullptr;
}