#pragma once
#include <QString>
#include <QVector>
#include <QPointF>
#include <QRectF>
#include <QColor>
#include <QStringList> 

#include "Config.h"
#include "SimulationLoader.h"
// 引入基础类型定义
#include "Types.h" 

class MapData {
public:
    MapData();

    // --- 配置 ---
    void setConfig(const AppConfig& config) { m_config = config; }

    // --- 数据访问 (Getters) ---
    const QVector<Edge>& edges() const { return m_edges; }
    const QVector<Junction>& junctions() const { return m_junctions; }
    const QVector<Vehicle>& vehicles() const { return m_vehicles; }
    QRectF bounds() const { return m_bounds; }
    QPointF getJunctionPosition(const QString& id);

    // --- 数据修改 (Setters) ---
    void clear();
    void addEdge(const Edge& edge) { m_edges.append(edge); }
    void addJunction(const Junction& junc) { m_junctions.append(junc); }
    void setBounds(const QRectF& rect) { m_bounds = rect; }

    void setVehicles(const QVector<Vehicle>& vehs) { m_vehicles = vehs; }
    QVector<Vehicle>& mutableVehicles() { return m_vehicles; }

    // --- 仿真控制 ---
    void initPlayback(const QString& directoryPath);
    void updateSimulationStep();
    void reset();

private:
    QVector<Edge> m_edges;
    QVector<Junction> m_junctions;
    QVector<Vehicle> m_vehicles;
    QRectF m_bounds;
    AppConfig m_config;

    // Loader 对象
    SimulationLoader m_simLoader;
    int m_currentFrameIndex = 0;
    int m_stepCounter = 0;
};