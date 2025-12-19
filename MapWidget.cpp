#include "MapWidget.h"
#include <QMouseEvent>
#include <algorithm> // for std::max    
// 链接 OpenGL 库 (防止之前的链接错误)
#pragma comment(lib, "opengl32.lib")

MapWidget::MapWidget(QWidget* parent)
    : QOpenGLWidget(parent), m_scale(1.0f), m_centerX(0), m_centerY(0)
{
    setFocusPolicy(Qt::StrongFocus);
}

MapWidget::~MapWidget() {}


void MapWidget::setRotation(float angle) {
    m_rotation = angle;
    update(); // 触发重绘
}

void MapWidget::setFocus(float x, float y, float zoomVal) {
    m_centerX = x;
    m_centerY = y;
    m_scale = zoomVal;
    update();
}

// 注入数据
void MapWidget::setData(std::shared_ptr<MapData> data) {
    m_data = data;
    fitMap(); // 数据来了，自动适配视角
}

void MapWidget::fitMap() {
    if (!m_data) return;

    QRectF bounds = m_data->bounds();
    m_centerX = bounds.center().x();
    m_centerY = bounds.center().y();

    // 简单的缩放估算
    float w = bounds.width();
    float h = bounds.height();
    if (w > 0 && h > 0) {
        m_scale = 600.0f / std::max(w, h) * 0.9f;
    }
    update();
}

void MapWidget::initializeGL() {
    initializeOpenGLFunctions();
    // 修改背景色为浅绿色，符合你的UI设计图
    glClearColor(0.85f, 0.93f, 0.85f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void MapWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void MapWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 如果没有数据，直接返回，不画图
    if (!m_data) return;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (m_scale <= 0.001f) m_scale = 0.001f;

    float viewHalfW = (width() / m_scale) / 2.0f;
    float viewHalfH = (height() / m_scale) / 2.0f;
    glOrtho(-viewHalfW, viewHalfW, -viewHalfH, viewHalfH, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(m_rotation, 0.0f, 0.0f, 1.0f);
    glTranslatef(-m_centerX, -m_centerY, 0.0f);

    // 1. 绘制路口 (深灰色)
    glColor3f(0.5f, 0.5f, 0.5f);
    for (const auto& junc : m_data->junctions()) {
        if (!junc.shape.empty()) {
            glBegin(GL_POLYGON);
            for (const auto& pt : junc.shape) glVertex2f(pt.x(), pt.y());
            glEnd();
        }
    }

    // 2. 绘制道路
    for (const auto& edge : m_data->edges()) {
        if (edge.function == "internal") {
            glColor3f(0.6f, 0.8f, 0.6f); // 内部线稍微淡一点
            glLineWidth(1.0f);
        }
        else {
            glColor3f(0.6f, 0.6f, 0.6f); // 普通道路改为灰色，非白色
            glLineWidth(3.0f); // 暂时加宽
        }

        for (const auto& lane : edge.lanes) {
            glBegin(GL_LINE_STRIP);
            for (const auto& pt : lane.shape) glVertex2f(pt.x(), pt.y());
            glEnd();
        }
    }
    for (const auto& veh : m_data->vehicles()) {
        glPushMatrix(); // 保存当前坐标系状态

        // 1. 移动到车辆位置
        glTranslatef(veh.x, veh.y, 0.0f);

        // 2. 旋转 (注意：SUMO 的 0 度通常是向北，OpenGL 默认向右，可能需要根据实际情况调整)
        // SUMO 角度通常是顺时针，OpenGL 是逆时针，这里取负值
        glRotatef(-veh.angle, 0.0f, 0.0f, 1.0f);

        // 3. 设置颜色
        glColor3f(veh.color.redF(), veh.color.greenF(), veh.color.blueF());

        // 4. 绘制矩形 (以车辆中心为原点)
        float halfLen = veh.length / 2.0f;
        float halfWid = veh.width / 2.0f;

        glRectf(-halfWid, -halfLen, halfWid, halfLen);

        glPopMatrix(); // 恢复坐标系，准备画下一辆车
    }
}

// 鼠标事件
void MapWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) m_lastMousePos = event->pos();
}

void MapWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        m_centerX -= delta.x() / m_scale;
        m_centerY += delta.y() / m_scale;
        update();
    }
}

void MapWidget::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0) m_scale *= 1.1f;
    else m_scale /= 1.1f;
    update();
}