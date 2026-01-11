#pragma once
#include <QPoint>
#include <QPointF>
#include <QtMath>

class MapCamera {
public:
    MapCamera();

    // --- 设置与状态 ---
    void setViewport(int w, int h); // 窗口大小改变时调用
    void setFocus(float x, float y, float zoomVal); // 聚焦到某点
    void set3D(bool enable);
    bool is3D() const { return m_is3D; }

    // --- OpenGL 矩阵应用 ---
    // 在 paintGL 中调用，负责设定 glFrustum 或 glOrtho
    void applyProjection();
    // 在 paintGL 中调用，负责设定 glTranslate/glRotate
    void applyModelView();

    // --- 交互逻辑 ---
    void pan(const QPoint& delta);       // 平移 (左键)
    void rotate(const QPoint& delta);    // 旋转 (右键)
    void zoom(int angleDelta);           // 缩放 (滚轮)

    // --- 核心数学计算 ---
    // 将屏幕像素坐标 (x,y) 转换为地图世界坐标 (x,y)
    QPointF screenToWorld(const QPoint& screenPos);

private:
    // 内部辅助：透视投影设置
    void setPerspectiveProjection();

private:
    int m_viewportW = 100;
    int m_viewportH = 100;

    // 摄像机参数
    float m_scale = 1.0f;
    float m_centerX = 0.0f;
    float m_centerY = 0.0f;

    // 旋转参数
    float m_rotation = 0.0f; // Z轴旋转 (2D/3D 通用)
    float m_pitch = 0.0f;    // X轴俯仰 (仅3D)
    float m_cameraZ = 200.0f;// 摄像机高度 (仅3D)

    bool m_is3D = false;
};