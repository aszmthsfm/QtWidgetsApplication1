#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <memory>
#include "MapData.h"
#include "Config.h"
#include "MapCamera.h"
#include "MapRenderer.h"

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

    // 代理函数
    void setRotation(float angle);
    void setFocus(float x, float y, float zoomVal);
    void set3D(bool enable);

signals:
    void selectionChanged();
    void vehicleSelected(const QString& info);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    std::shared_ptr<MapData> m_data;
    AppConfig m_config;
    RenderStyle m_style = STYLE_FLAT;


    MapCamera m_camera;      // 负责看 (数学/交互)
    MapRenderer m_renderer;  // 负责画 (OpenGL)

    QPoint m_lastMousePos;
    QPoint m_pressPos;

};