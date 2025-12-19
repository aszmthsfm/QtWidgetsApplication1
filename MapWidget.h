#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <memory> //用于 std::shared_ptr
#include "MapData.h" // 包含数据定义

class MapWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    MapWidget(QWidget* parent = nullptr);
    ~MapWidget();

    // 【关键修改】不再加载文件，而是接收数据指针
    void setData(std::shared_ptr<MapData> data);

    // 重置视角以适应地图
    void fitMap();
    // 设置旋转角度 (单位：度)
    void setRotation(float angle);

    // 强制聚焦到某个点，并设置缩放等级 (zoomVal越大看的内容越少/越近)
    void setFocus(float x, float y, float zoomVal);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    // 使用智能指针共享数据
    std::shared_ptr<MapData> m_data;

    float m_scale;
    float m_centerX;
    float m_centerY;
    QPoint m_lastMousePos;
    float m_rotation = 0.0f;
};