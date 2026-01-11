#include "MapWidget.h"
#include <QMouseEvent>
#include <algorithm>
#include <cmath>
#include <QVector3D>
#include <QMatrix4x4>

#pragma comment(lib, "opengl32.lib")

MapWidget::MapWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

MapWidget::~MapWidget() {
    // 资源释放由 MapRenderer 的析构函数自动处理，这里不需要管了
}

float checkVehicleIntersection(const Ray& ray, const Vehicle& veh) {
    // 1. 定义车辆尺寸
    float len = veh.length;
    float halfW = veh.width / 2.0f;
    float h = 1.6f;

    // 2. 将射线变换到车辆的局部坐标系
    QMatrix4x4 toLocal;
    toLocal.rotate(veh.angle, 0.0f, 0.0f, 1.0f);
    toLocal.translate(-veh.x, -veh.y, -0.1f);
    QVector3D rayOriginLocal = toLocal.map(ray.origin);       // 变换点：应用位移
    QVector3D rayDirLocal = toLocal.mapVector(ray.direction); // 变换向量：不应用位移

    // 3. 定义车辆的 AABB
    QVector3D boxMin(-halfW, -len, 0.0f);
    QVector3D boxMax(halfW, 0.0f, h);

    // 4. 标准 Slab 方法检测
    float tMin = 0.0f;
    float tMax = 100000.0f;

    // X轴检测
    if (std::abs(rayDirLocal.x()) < 1e-6) {
        if (rayOriginLocal.x() < boxMin.x() || rayOriginLocal.x() > boxMax.x()) return -1.0f;
    }
    else {
        float t1 = (boxMin.x() - rayOriginLocal.x()) / rayDirLocal.x();
        float t2 = (boxMax.x() - rayOriginLocal.x()) / rayDirLocal.x();
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) return -1.0f;
    }

    // Y轴检测
    if (std::abs(rayDirLocal.y()) < 1e-6) {
        if (rayOriginLocal.y() < boxMin.y() || rayOriginLocal.y() > boxMax.y()) return -1.0f;
    }
    else {
        float t1 = (boxMin.y() - rayOriginLocal.y()) / rayDirLocal.y();
        float t2 = (boxMax.y() - rayOriginLocal.y()) / rayDirLocal.y();
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) return -1.0f;
    }

    // Z轴检测
    if (std::abs(rayDirLocal.z()) < 1e-6) {
        if (rayOriginLocal.z() < boxMin.z() || rayOriginLocal.z() > boxMax.z()) return -1.0f;
    }
    else {
        float t1 = (boxMin.z() - rayOriginLocal.z()) / rayDirLocal.z();
        float t2 = (boxMax.z() - rayOriginLocal.z()) / rayDirLocal.z();
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) return -1.0f;
    }

    return tMin;
}

void MapWidget::setRotation(float angle) {
    m_camera.setRotation(angle);
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

    glEnable(GL_DEPTH_TEST);
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

// --- 交互部分---

void MapWidget::mousePressEvent(QMouseEvent* event) {
    // 1. 记录当前位置，供 mouseMoveEvent 计算 delta 使用 
    m_lastMousePos = event->pos();
    // 2. 记录按下的初始位置，用于 mouseReleaseEvent 判断是否是点击
    m_pressPos = event->pos();
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

void MapWidget::mouseReleaseEvent(QMouseEvent* event) {
    QPoint dist = event->pos() - m_pressPos;
    int moveDist = std::abs(dist.x()) + std::abs(dist.y());

    if (moveDist < 5) {
        if (event->button() == Qt::LeftButton && m_data) {
            Ray pickRay = m_camera.getRay(event->pos());

            float minDistance = 10000.0f;
            bool found = false;
            QString selectedId = "";

            // 遍历车辆进行检测
            for (const auto& veh : m_data->vehicles()) {
                float hitDist = checkVehicleIntersection(pickRay, veh);
                if (hitDist > 0 && hitDist < minDistance) {
                    minDistance = hitDist;
                    found = true;
                    selectedId = veh.id;
                }
            }

            // 更新状态并发送信号
            if (found) {
                m_data->setSelectedVehicleId(selectedId);
            }
            else {
                m_data->setSelectedVehicleId("");
            }

            // 发送信号
            emit selectionChanged();

            update();
        }
    }
}

void MapWidget::wheelEvent(QWheelEvent* event) {
    m_camera.zoom(event->angleDelta().y());
    update();
}

