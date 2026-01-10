#pragma once
#include <QString>
#include <QColor>
#include <QVector>

// 1. 窗口与UI配置
struct WindowConfig {
    int width = 1600;
    int height = 900;
    QString title = "SUMO Dashboard (Qt6 + OpenGL)";
};

// 2. 仿真相关配置
struct SimulationConfig {
    int targetFPS = 5;       // 默认FPS
    int vehicleCount = 20;   // 生成车辆数量
};

// 3. 车辆参数配置
struct VehicleConfig {
    float length = 5.0f;
    float width = 2.0f;
    float minSpeed = 0.8f;
    float maxSpeed = 1.5f;
    // 车辆可选颜色池
    QVector<QColor> colors = {
        Qt::red, Qt::blue, Qt::green, Qt::cyan,
        Qt::magenta, Qt::yellow, Qt::darkRed
    };
};

// 4. 地图与渲染配置
struct MapConfig {
    QString netFilePath = "road.net.xml"; // 路网文件路径

    // 颜色配置
    QColor backgroundColor = QColor(217, 237, 217); // 0.85, 0.93, 0.85
    QColor junctionColor = QColor(128, 128, 128);   // 0.5, 0.5, 0.5
    QColor internalLaneColor = QColor(153, 204, 153); // 0.6, 0.8, 0.6
    QColor roadColor = QColor(153, 153, 153);       // 0.6, 0.6, 0.6

    // 线宽与缩放
    float internalLineWidth = 1.0f;
    float roadLineWidth = 3.0f;
    float defaultLocalZoom = 5.0f; // 局部路口的缩放等级
};

// --- 总配置结构体 ---
struct AppConfig {
    WindowConfig window;
    SimulationConfig sim;
    VehicleConfig vehicle;
    MapConfig map;
};