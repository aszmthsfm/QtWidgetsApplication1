#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <memory>
#include <QVector2D>
#include "MapData.h"
#include "Config.h"

class MapWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    enum RenderStyle {
        STYLE_FLAT,
        STYLE_REALISTIC
    };

    MapWidget(QWidget* parent = nullptr);
    ~MapWidget();

    void setData(std::shared_ptr<MapData> data);
    void setConfig(const AppConfig& config) { m_config = config; }
    void setStyle(RenderStyle style) { m_style = style; update(); }

    void fitMap();
    void setRotation(float angle);
    void setFocus(float x, float y, float zoomVal);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void drawRealisticRoads();
    void drawFlatRoads();

    // 绘制路面面片
    void drawWideLane(const QVector<QPointF>& shape, float width, QColor color);

    // 绘制实线（边界、中心线）
    void renderSingleLine(const QVector<QPointF>& shape, float offsetValue);

    // 绘制几何虚线（解决缩放问题，长度基于地图米）
    void renderGeometricDashedLine(const QVector<QPointF>& shape, float offsetValue, float dashLen, float gapLen);

    // 绘制停止线（密集竖线，带边界过滤）
    void drawStopLine(const QVector<QPointF>& shape, float laneWidth, bool isEnd);

    std::shared_ptr<MapData> m_data;
    AppConfig m_config;

    RenderStyle m_style = STYLE_FLAT;
    QOpenGLTexture* m_roadTexture = nullptr;

    float m_scale;
    float m_centerX;
    float m_centerY;
    QPoint m_lastMousePos;
    float m_rotation = 0.0f;
};