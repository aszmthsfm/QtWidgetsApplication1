#pragma once
#include <QString>
#include <QVector>
#include <QPointF>
#include <QRectF>
#include <QColor>
#include <QRandomGenerator>
#include <QStringList> 
#include "Config.h"
#include <QMap>

struct Lane {
    QString id;
    float width;
    QVector<QPointF> shape;
};

struct Edge {
    QString id;
    QString fromJunc;
    QString toJunc;
    QString function;
    QVector<Lane> lanes;
};

struct Junction {
    QString id;
    QVector<QPointF> shape;
};

struct Vehicle {
    QString id;
    float x;
    float y;
    float angle;  // 对应 JSON 中的 heading
    float length;
    float width;
    QColor color;

    // 以下字段在回放模式下可能仅用于显示，不再用于物理计算
    QString currentEdgeId;
    int currentLaneIndex;
    int currentShapeIndex;
    float speed;
    int stuckFrames = 0;

    //失联帧数计数器
    int missingFrames = 0;
};

class MapData {
public:
    MapData();
    void setConfig(const AppConfig& config) { m_config = config; }
    bool load(const QString& filePath);
    void initPlayback(const QString& directoryPath);

    const QVector<Edge>& edges() const { return m_edges; }
    const QVector<Junction>& junctions() const { return m_junctions; }
    const QVector<Vehicle>& vehicles() const { return m_vehicles; }
    QRectF bounds() const { return m_bounds; }

    void updateSimulationStep();
    QPointF getJunctionPosition(const QString& id);

    void reset();

private:
    void parseShape(const QString& shapeStr, QVector<QPointF>& outPoints);
    void spawnVehicle();

    QVector<Edge> m_edges;
    QVector<Junction> m_junctions;
    QVector<Vehicle> m_vehicles;
    QRectF m_bounds;
    AppConfig m_config;
    int m_stepCounter = 0;

    QString m_dataDir;
    QStringList m_jsonFiles;
    int m_currentFrameIndex = 0;

    // 颜色缓存：Key是车辆ID，Value是颜色
    //QMap<QString, QColor> m_vehicleColorCache;
};