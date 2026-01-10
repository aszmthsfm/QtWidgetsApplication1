#include "MapWidget.h"
#include <QMouseEvent>
#include <algorithm>
#include <QDebug>
#include <cmath>

#pragma comment(lib, "opengl32.lib")

MapWidget::MapWidget(QWidget* parent)
    : QOpenGLWidget(parent), m_scale(1.0f), m_centerX(0), m_centerY(0)
{
    setFocusPolicy(Qt::StrongFocus);
}

MapWidget::~MapWidget() {
    if (m_roadTexture) {
        delete m_roadTexture;
        m_roadTexture = nullptr;
    }
}

void MapWidget::setRotation(float angle) {
    m_rotation = angle;
    update();
}

void MapWidget::setFocus(float x, float y, float zoomVal) {
    m_centerX = x;
    m_centerY = y;
    m_scale = zoomVal;
    update();
}

void MapWidget::setData(std::shared_ptr<MapData> data) {
    m_data = data;
    fitMap();
}

void MapWidget::fitMap() {
    if (!m_data) return;
    QRectF bounds = m_data->bounds();
    m_centerX = bounds.center().x();
    m_centerY = bounds.center().y();
    float w = (float)bounds.width();
    float h = (float)bounds.height();
    if (w > 0 && h > 0) {
        m_scale = 600.0f / std::max(w, h) * 0.9f;
    }
    update();
}

void MapWidget::initializeGL() {
    initializeOpenGLFunctions();
    QColor bg = m_config.map.backgroundColor;
    glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    QImage img("asphalt.jpg");
    if (!img.isNull()) {
        m_roadTexture = new QOpenGLTexture(img.mirrored());
        m_roadTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_roadTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_roadTexture->setWrapMode(QOpenGLTexture::Repeat);
    }
}

void MapWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

// MapWidget.cpp

void MapWidget::paintGL() {
    // 1. 设置背景色 (保持原有逻辑)
    QColor bg = m_config.map.backgroundColor;
    if (m_style == STYLE_REALISTIC) {
        glClearColor(0.13f, 0.35f, 0.13f, 1.0f);
    }
    else {
        glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_data) return;

    // =========================================================
    // 【核心修改】投影矩阵设置
    // =========================================================
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (m_is3D) {
        // --- 3D 模式：使用透视投影 (Perspective) ---
        // 近大远小，有消失点
        setPerspectiveProjection(width(), height());
    }
    else {
        // --- 2D 模式：使用正交投影 (Orthographic) ---
        // 保持原有逻辑
        if (m_scale <= 0.001f) m_scale = 0.001f;
        float viewHalfW = (width() / m_scale) / 2.0f;
        float viewHalfH = (height() / m_scale) / 2.0f;
        glOrtho(-viewHalfW, viewHalfW, -viewHalfH, viewHalfH, -2000.0, 2000.0);
    }

    // =========================================================
    // 【核心修改】模型视图矩阵 (摄像机位置)
    // =========================================================
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (m_is3D) {
        // 3D 摄像机变换顺序：
        // 1. 移远 (Z轴拉开距离)
        glTranslatef(0.0f, 0.0f, -m_cameraZ);

        // 2. 俯仰 (绕X轴转，抬头低头)
        glRotatef(-m_pitch, 1.0f, 0.0f, 0.0f);

        // 3. 旋转 (绕Z轴转，东南西北)
        glRotatef(m_rotation, 0.0f, 0.0f, 1.0f);

        // 4. 移动到地图中心
        glTranslatef(-m_centerX, -m_centerY, 0.0f);
    }
    else {
        // 2D 摄像机变换 (保持原有)
        glRotatef(m_rotation, 0.0f, 0.0f, 1.0f);
        glTranslatef(-m_centerX, -m_centerY, 0.0f);
    }

    // =========================================================
    // 绘制内容 (保持不变)
    // =========================================================
    if (m_style == STYLE_REALISTIC) drawRealisticRoads();
    else drawFlatRoads();

    // 绘制车辆 (保持不变)
    for (const auto& veh : m_data->vehicles()) {
        glPushMatrix();
        glTranslatef(veh.x, veh.y, 0.0f);
        glRotatef(-veh.angle, 0.0f, 0.0f, 1.0f);
        glColor3f(veh.color.redF(), veh.color.greenF(), veh.color.blueF());
        float len = veh.length;
        float halfWid = veh.width / 2.0f;

        // 画个简单的立体盒子车
        if (m_is3D) {
            // 车顶
            glBegin(GL_QUADS);
            glVertex3f(-halfWid, -len, 1.5f);
            glVertex3f(halfWid, -len, 1.5f);
            glVertex3f(halfWid, 0.0f, 1.5f);
            glVertex3f(-halfWid, 0.0f, 1.5f);
            glEnd();
            // (为了简化代码，侧面暂时省略，或者你可以自己加 GL_QUADS 画侧面)
        }

        // 原始的 2D 车身
        glRectf(-halfWid, -len, halfWid, 0.0f);

        // 黑色轮廓
        glColor3f(0, 0, 0);
        glLineWidth(1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(-halfWid, -len);
        glVertex2f(halfWid, -len);
        glVertex2f(halfWid, 0.0f);
        glVertex2f(-halfWid, 0.0f);
        glEnd();
        glPopMatrix();
    }
}

// 【新增】透视投影辅助函数
void MapWidget::setPerspectiveProjection(int w, int h) {
    if (h == 0) h = 1;
    float aspectRatio = (float)w / (float)h;
    float fov = 45.0f; // 视野角度
    float zNear = 1.0f;
    float zFar = 10000.0f; //以此保证能看得很远

    // 手动计算 glFrustum 参数
    float top = zNear * std::tan(fov * 0.5f * 3.1415926f / 180.0f);
    float bottom = -top;
    float right = top * aspectRatio;
    float left = -right;

    glFrustum(left, right, bottom, top, zNear, zFar);
}

void MapWidget::drawWideLane(const QVector<QPointF>& shape, float width, QColor color) {
    if (shape.size() < 2) return;
    float halfW = width / 2.0f;
    glColor3f(color.redF(), color.greenF(), color.blueF());
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i < shape.size(); ++i) {
        QVector2D normal;
        QPointF p = shape[i];
        if (i < shape.size() - 1) {
            QVector2D dir(shape[i + 1].x() - p.x(), shape[i + 1].y() - p.y());
            dir.normalize();
            normal = QVector2D(-dir.y(), dir.x());
        }
        else {
            QVector2D dir(p.x() - shape[i - 1].x(), p.y() - shape[i - 1].y());
            dir.normalize();
            normal = QVector2D(-dir.y(), dir.x());
        }
        if (i > 0 && i < shape.size() - 1) {
            QVector2D d1(shape[i].x() - shape[i - 1].x(), shape[i].y() - shape[i - 1].y());
            QVector2D d2(shape[i + 1].x() - shape[i].x(), shape[i + 1].y() - shape[i].y());
            d1.normalize(); d2.normalize();
            QVector2D n1(-d1.y(), d1.x()); QVector2D n2(-d2.y(), d2.x());
            normal = (n1 + n2).normalized();
        }
        QVector2D offset = normal * halfW;
        QPointF pL(p.x() - offset.x(), p.y() - offset.y());
        QPointF pR(p.x() + offset.x(), p.y() + offset.y());
        glTexCoord2f(pL.x() * 0.2f, pL.y() * 0.2f); glVertex2f(pL.x(), pL.y());
        glTexCoord2f(pR.x() * 0.2f, pR.y() * 0.2f); glVertex2f(pR.x(), pR.y());
    }
    glEnd();
}

void MapWidget::renderSingleLine(const QVector<QPointF>& shape, float offsetValue) {
    glBegin(GL_LINE_STRIP);
    for (int j = 0; j < shape.size(); ++j) {
        QPointF p = shape[j];
        QVector2D dir;
        if (j < shape.size() - 1) dir = QVector2D(shape[j + 1].x() - p.x(), shape[j + 1].y() - p.y());
        else dir = QVector2D(p.x() - shape[j - 1].x(), p.y() - shape[j - 1].y());
        dir.normalize();
        QVector2D normal(-dir.y(), dir.x());
        if (j > 0 && j < shape.size() - 1) {
            QVector2D d1(shape[j].x() - shape[j - 1].x(), shape[j].y() - shape[j - 1].y());
            QVector2D d2(shape[j + 1].x() - shape[j].x(), shape[j + 1].y() - shape[j].y());
            d1.normalize(); d2.normalize();
            QVector2D n1(-d1.y(), d1.x()); QVector2D n2(-d2.y(), d2.x());
            normal = (n1 + n2).normalized();
        }
        QPointF pt(p.x() + normal.x() * offsetValue, p.y() + normal.y() * offsetValue);
        glVertex2f(pt.x(), pt.y());
    }
    glEnd();
}

// 核心修改：几何虚线绘制 
void MapWidget::renderGeometricDashedLine(const QVector<QPointF>& shape, float offsetValue, float dashLen, float gapLen) {
    if (shape.size() < 2) return;
    float totalPatternLen = dashLen + gapLen;
    // 1. 获取第一段路的方向向量
    QVector2D dir(shape[1].x() - shape[0].x(), shape[1].y() - shape[0].y());
    if (dir.lengthSquared() > 0.0001f) {
        dir.normalize();
    }
    else {
        dir = QVector2D(1.0f, 0.0f); // 防止零向量
    }

    // 2. 将起点坐标投影到道路方向上 (Dot Product)
    float worldProjection = shape[0].x() * dir.x() + shape[0].y() * dir.y();

    // 3. 取模得到初始相位 (处理负数情况，确保相位为正)
    float distanceAlongPath = fmod(worldProjection, totalPatternLen);
    if (distanceAlongPath < 0.0f) {
        distanceAlongPath += totalPatternLen;
    }
    // ==============================================================

    glBegin(GL_LINES);
    for (int i = 0; i < shape.size() - 1; ++i) {
        QPointF p1 = shape[i];
        QPointF p2 = shape[i + 1];
        QVector2D dirVec(p2.x() - p1.x(), p2.y() - p1.y());
        float segmentLen = dirVec.length();
        dirVec.normalize();
        QVector2D normal(-dirVec.y(), dirVec.x());

        float currentPos = 0.0f;
        while (currentPos < segmentLen) {
            // 使用 fmod 循环绘制
            float relPos = fmod(distanceAlongPath + currentPos, totalPatternLen);

            if (relPos < dashLen) {
                // 当前在“实线”区间，需要绘制
                float drawLen = std::min(dashLen - relPos, segmentLen - currentPos);

                // 处理跨越周期边界的情况 (例如 relPos=3, dashLen=5, 只能画2米)
                // 但上面的 min 逻辑已经涵盖了，这里主要是为了跳过 gap

                QPointF v1(p1.x() + normal.x() * offsetValue + dirVec.x() * currentPos,
                    p1.y() + normal.y() * offsetValue + dirVec.y() * currentPos);

                QPointF v2(p1.x() + normal.x() * offsetValue + dirVec.x() * (currentPos + drawLen),
                    p1.y() + normal.y() * offsetValue + dirVec.y() * (currentPos + drawLen));

                glVertex2f(v1.x(), v1.y());
                glVertex2f(v2.x(), v2.y());

                currentPos += drawLen;
            }
            else {
                // 当前在“间隔”区间，跳过
                float skipLen = std::min(totalPatternLen - relPos, segmentLen - currentPos);
                currentPos += skipLen;
            }
        }
        // 累加整段路程，保证下一段路的相位连续
        distanceAlongPath += segmentLen;
    }
    glEnd();
}


void MapWidget::drawStopLine(const QVector<QPointF>& shape, float laneWidth, bool isEnd) {
    if (shape.size() < 2 || !m_data) return;
    QPointF basePt = isEnd ? shape.last() : shape.first();

    // 边界过滤
    QRectF bounds = m_data->bounds();
    float eps = 1.5f;
    if (basePt.x() <= bounds.left() + eps || basePt.x() >= bounds.right() - eps ||
        basePt.y() <= bounds.top() + eps || basePt.y() >= bounds.bottom() - eps) {
        return;
    }

    glDisable(GL_LINE_STIPPLE);
    QVector2D roadDir;
    if (isEnd) roadDir = QVector2D(basePt.x() - shape[shape.size() - 2].x(), basePt.y() - shape[shape.size() - 2].y()).normalized();
    else roadDir = QVector2D(shape[1].x() - basePt.x(), shape[1].y() - basePt.y()).normalized();

    QVector2D normal(-roadDir.y(), roadDir.x());
    float halfW = laneWidth / 2.0f;
    float stripeLen = 1.2f;
    int stripeCount = 3;

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    for (int i = 0; i <= stripeCount; ++i) {
        float t = -1.0f + 2.0f * i / stripeCount;
        QPointF pS(basePt.x() + normal.x() * halfW * t, basePt.y() + normal.y() * halfW * t);
        float sign = isEnd ? -1.0f : 1.0f;
        QPointF pE(pS.x() + roadDir.x() * stripeLen * sign, pS.y() + roadDir.y() * stripeLen * sign);
        glVertex2f(pS.x(), pS.y());
        glVertex2f(pE.x(), pE.y());
    }
    glEnd();
}

void MapWidget::drawRealisticRoads() {
    glEnable(GL_TEXTURE_2D);
    if (m_roadTexture) m_roadTexture->bind();
    glColor3f(0.25f, 0.25f, 0.25f);
    for (const auto& junc : m_data->junctions()) {
        if (!junc.shape.empty()) {
            glBegin(GL_POLYGON);
            for (const auto& pt : junc.shape) {
                glTexCoord2f(pt.x() * 0.2f, pt.y() * 0.2f); glVertex2f(pt.x(), pt.y());
            }
            glEnd();
        }
    }
    for (const auto& edge : m_data->edges()) {
        if (edge.function == "internal") continue;
        for (const auto& lane : edge.lanes) {
            drawWideLane(lane.shape, lane.width, QColor::fromRgbF(0.25f, 0.25f, 0.25f));
        }
    }
    if (m_roadTexture) m_roadTexture->release();
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    for (const auto& edge : m_data->edges()) {
        if (edge.function == "internal") continue;
        int laneCount = edge.lanes.size();
        for (int i = 0; i < laneCount; ++i) {
            const Lane& lane = edge.lanes[i];
            float hW = lane.width / 2.0f;

            if (i < laneCount - 1) {
                if (i == 2) { // 黄色中心实线
                    glColor3f(1.0f, 1.0f, 0.0f); glDisable(GL_LINE_STIPPLE); glLineWidth(3.5f);
                    renderSingleLine(lane.shape, hW);
                }
                else { // 稀疏几何虚线
                    glColor3f(1.0f, 1.0f, 1.0f); glDisable(GL_LINE_STIPPLE); glLineWidth(1.5f);
                    renderGeometricDashedLine(lane.shape, hW, 4.0f, 6.0f);
                }
            }

            if (i == 0 || i == laneCount - 1) { // 边缘实线
                glColor3f(0.85f, 0.85f, 0.85f); glDisable(GL_LINE_STIPPLE); glLineWidth(2.0f);
                renderSingleLine(lane.shape, (i == 0 ? -hW : hW));
            }

            drawStopLine(lane.shape, lane.width, true);
            drawStopLine(lane.shape, lane.width, false);
        }
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
    glLineWidth(1.0f);
}

void MapWidget::drawFlatRoads() {
    QColor roadColor = m_config.map.roadColor;
    for (const auto& edge : m_data->edges()) {
        glColor3f(roadColor.redF(), roadColor.greenF(), roadColor.blueF());
        glLineWidth(m_config.map.roadLineWidth);
        for (const auto& lane : edge.lanes) {
            glBegin(GL_LINE_STRIP);
            for (const auto& pt : lane.shape) glVertex2f(pt.x(), pt.y());
            glEnd();
        }
    }
}

void MapWidget::mousePressEvent(QMouseEvent* event) { if (event->button() == Qt::LeftButton) m_lastMousePos = event->pos(); }

// MapWidget.cpp -> mouseMoveEvent

void MapWidget::mouseMoveEvent(QMouseEvent* event) {
    QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();

    // --- 左键：平移 (Pan) ---
    if (event->buttons() & Qt::LeftButton) {
        float rad = m_rotation * 3.1415926f / 180.0f;
        float cosA = cos(-rad);
        float sinA = sin(-rad);
        float dx = delta.x();
        float dy = delta.y();

        // 3D 模式下，平移速度需要根据高度动态调整，否则拉远了拖不动
        float panScale = m_is3D ? (m_cameraZ / 600.0f) : (1.0f / m_scale);

        // 考虑旋转对平移方向的影响
        float moveX = dx * cosA - dy * sinA;
        float moveY = dx * sinA + dy * cosA;

        m_centerX -= moveX * panScale;
        m_centerY += moveY * panScale;
        update();
    }

    // --- 右键：旋转 (Rotate) ---
    if (event->buttons() & Qt::RightButton) {
        if (m_is3D) {
            // 左右拖拽 -> 改变方向 (Z轴旋转)
            m_rotation += delta.x() * 0.5f;

            // 上下拖拽 -> 改变俯仰角 (X轴旋转)
            m_pitch += delta.y() * 0.5f;

            // 限制俯仰角，防止翻转 (0度是平视，90度是垂直俯视)
            if (m_pitch < 10.0f) m_pitch = 10.0f;
            if (m_pitch > 89.0f) m_pitch = 89.0f;
        }
        else {
            // 2D 模式下右键只允许平面旋转
            m_rotation += delta.x() * 0.5f;
        }
        update();
    }
}

// MapWidget.cpp -> wheelEvent

void MapWidget::wheelEvent(QWheelEvent* event) {
    int delta = event->angleDelta().y();

    if (m_is3D) {
        // --- 3D 模式：滚轮改变摄像机高度 (Zoom) ---
        if (delta > 0) m_cameraZ *= 0.9f; // 拉近
        else m_cameraZ *= 1.1f;           // 拉远

        // 限制高度范围
        if (m_cameraZ < 10.0f) m_cameraZ = 10.0f;
        if (m_cameraZ > 5000.0f) m_cameraZ = 5000.0f;
    }
    else {
        // --- 2D 模式：滚轮改变 Scale ---
        if (delta > 0) m_scale *= 1.1f;
        else m_scale /= 1.1f;
    }
    update();
}