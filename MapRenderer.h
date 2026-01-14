#pragma once
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QColor>
#include <memory>
#include "MapData.h"
#include "Config.h"

class MapRenderer : protected QOpenGLFunctions {
public:
    MapRenderer();
    ~MapRenderer();

    // 初始化 OpenGL 资源 ，必须在 OpenGL 上下文中调用
    void initialize();

    // 核心绘制函数
    // 传入：数据、配置、当前是否 3D 模式、渲染风格
    void render(const std::shared_ptr<MapData>& data,
        const AppConfig& config,
        bool is3D,
        int style);

private:
    // --- 内部绘制实现 ---
    void drawRealisticRoads(const std::shared_ptr<MapData>& data, const AppConfig& config);
    void drawFlatRoads(const std::shared_ptr<MapData>& data, const AppConfig& config);
    void drawVehicles(const std::shared_ptr<MapData>& data, bool is3D);

    // 辅助绘制函数
    void drawWideLane(const QVector<QPointF>& shape, float width, QColor color);
    void renderSingleLine(const QVector<QPointF>& shape, float offsetValue);
    void renderGeometricDashedLine(const QVector<QPointF>& shape, float offsetValue, float dashLen, float gapLen);
    void drawStopLine(const std::shared_ptr<MapData>& data, const QVector<QPointF>& shape, float laneWidth, bool isEnd);
    void draw3DVehicle(float length, float width, QColor color);

private:
    QOpenGLTexture* m_roadTexture = nullptr;

    GLuint m_carList = 0;   // 轿车显示列表
    GLuint m_busList = 0;   // 大车显示列表

    // 辅助函数：加载 OBJ
    GLuint loadObjModel(const QString& filePath);
};