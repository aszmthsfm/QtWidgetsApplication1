#pragma once
#include <QString>
#include <QVector>
#include <QPointF>
#include <QRectF> // <--- 必须添加这一行，否则会报 "未定义类型 QRectF"
#include <QDomDocument>
#include <QFile>
#include <QDebug>
#include <QtGlobal>
#include <QColor>

// --- 基础数据结构 (从 MapWidget 移出来的) ---
struct Lane {
    QString id;
    float width;
    QVector<QPointF> shape;
};

struct Edge {
    QString id;
    QString function;
    QVector<Lane> lanes;
};

struct Junction {
    QString id;
    QVector<QPointF> shape;
};
//车辆结构体
struct Vehicle {
    QString id;
    float x;
    float y;
    float angle;  // 角度 (0-360)
    float length;
    float width;
    QColor color; // 车辆颜色 (红/蓝)
};

class MapData {
public:
    MapData();
    bool load(const QString& filePath);

    const QVector<Edge>& edges() const { return m_edges; }
    const QVector<Junction>& junctions() const { return m_junctions; }
    QRectF bounds() const { return m_bounds; }

    // --- 新增：车辆相关接口 ---
    const QVector<Vehicle>& vehicles() const { return m_vehicles; }

    // 用于测试：更新车辆位置 (模拟仿真)
    void updateSimulationStep() {
        // 这里我们先写死一辆车在地图上转圈，或者沿直线走，用来测试渲染
        // 实际项目中，这里应该解析 SUMO 的实时数据
        if (m_vehicles.isEmpty()) {
            // 初始化一辆测试车
            m_vehicles.append({ "test_car_1", 50, 50, 0, 5.0, 2.0, Qt::red });
        }

        // 让车简单的沿 Y 轴移动
        for (auto& veh : m_vehicles) {
            veh.y += 0.5f; // 每次移动 0.5 米
            if (veh.y > 200) veh.y = 0; // 循环
        }
    }
    // 根据 ID 获取路口的中心坐标
    QPointF getJunctionPosition(const QString& id) {
        for (const auto& junc : m_junctions) {
            if (junc.id == id) {
                // 计算多边形的几何中心
                float sumX = 0, sumY = 0;
                if (junc.shape.isEmpty()) return QPointF(0, 0);

                for (const auto& pt : junc.shape) {
                    sumX += pt.x();
                    sumY += pt.y();
                }
                return QPointF(sumX / junc.shape.size(), sumY / junc.shape.size());
            }
        }
        return QPointF(0, 0);
    }

private:
    void parseShape(const QString& shapeStr, QVector<QPointF>& outPoints);

    QVector<Edge> m_edges;
    QVector<Junction> m_junctions;
    // --- 新增：车辆列表 ---
    QVector<Vehicle> m_vehicles;
    QRectF m_bounds;
};