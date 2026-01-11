#pragma once
#include <QString>
#include <QVector>
#include <QPointF>
#include <QColor>

// --- 基础数据结构 ---
// 从 MapData.h 剥离出来，打破循环依赖

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

    // 以下字段在回放模式下可能仅用于显示
    QString currentEdgeId;
    int currentLaneIndex;
    int currentShapeIndex;
    float speed;
    int stuckFrames = 0;

    // 失联帧数计数器 (用于惯性导航)
    int missingFrames = 0;
};