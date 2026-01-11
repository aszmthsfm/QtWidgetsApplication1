#include "MapWidget.h"
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

#pragma comment(lib, "opengl32.lib")

MapWidget::MapWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

MapWidget::~MapWidget() {
    // 资源释放由 MapRenderer 的析构函数自动处理，这里不需要管了
}

void MapWidget::setRotation(float angle) {
    update();
}

void MapWidget::setFocus(float x, float y, float zoomVal) {
    m_camera.setFocus(x, y, zoomVal);
    update();
}

void MapWidget::set3D(bool enable) {
    m_camera.set3D(enable);
    update();
}

void MapWidget::setData(std::shared_ptr<MapData> data) {
    m_data = data;
    fitMap();
}

void MapWidget::fitMap() {
    if (!m_data) return;
    QRectF bounds = m_data->bounds();
    float w = (float)bounds.width();
    float h = (float)bounds.height();
    if (w > 0 && h > 0) {
        float scale = 600.0f / std::max(w, h) * 0.9f;
        m_camera.setFocus(bounds.center().x(), bounds.center().y(), scale);
    }
    update();
}

void MapWidget::initializeGL() {
    initializeOpenGLFunctions();

    // 1. 初始化渲染器 (加载纹理等)
    m_renderer.initialize();

    // 2. 只有 OpenGL 状态设置保留在这里，或者也可以移到 Renderer
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void MapWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    m_camera.setViewport(w, h);
}

void MapWidget::paintGL() {
    // 1. 设置背景 (简单逻辑保留在这里，或者给 Renderer 加一个 clearScreen 接口)
    QColor bg = m_config.map.backgroundColor;
    if (m_style == STYLE_REALISTIC) glClearColor(0.13f, 0.35f, 0.13f, 1.0f);
    else glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_data) return;

    // 2. 应用摄像机
    m_camera.applyProjection();
    m_camera.applyModelView();

    // 3. 【核心调用】命令渲染器干活
    // 传入所有需要的数据，MapWidget 不需要知道具体怎么画
    m_renderer.render(m_data, m_config, m_camera.is3D(), (int)m_style);
}

// --- 交互部分 (保持不变，因为这属于 Controller 逻辑) ---

void MapWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->pos();
        if (m_data) {
            QPointF worldPos = m_camera.screenToWorld(event->pos());

            // ... (点击检测逻辑保持不变) ...
            // 为节省篇幅，这里略去具体的查找循环，这部分逻辑建议未来移到 MapData::pickVehicle(pos)
            float minDistance = 10000.0f;
            bool found = false;
            QString info;
            for (const auto& veh : m_data->vehicles()) {
                float dx = veh.x - worldPos.x();
                float dy = veh.y - worldPos.y();
                if (std::sqrt(dx * dx + dy * dy) < 8.0f) {
                    found = true;
                    info = "Selected: " + veh.id;
                    break;
                }
            }
            if (found) emit vehicleSelected(info);
        }
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent* event) {
    QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();

    if (event->buttons() & Qt::LeftButton) {
        m_camera.pan(delta);
        update();
    }
    else if (event->buttons() & Qt::RightButton) {
        m_camera.rotate(delta);
        update();
    }
}

void MapWidget::wheelEvent(QWheelEvent* event) {
    m_camera.zoom(event->angleDelta().y());
    update();
}