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

void MapWidget::paintGL() {
    QColor bg = m_config.map.backgroundColor;
    if (m_style == STYLE_REALISTIC) {
        glClearColor(0.13f, 0.35f, 0.13f, 1.0f);
    }
    else {
        glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    if (m_style == STYLE_REALISTIC) drawRealisticRoads();
    else drawFlatRoads();

    // 绘制车辆
    for (const auto& veh : m_data->vehicles()) {
        glPushMatrix();
        glTranslatef(veh.x, veh.y, 0.0f);
        glRotatef(-veh.angle, 0.0f, 0.0f, 1.0f);
        glColor3f(veh.color.redF(), veh.color.greenF(), veh.color.blueF());
        float halfLen = veh.length / 2.0f;
        float halfWid = veh.width / 2.0f;
        glRectf(-halfWid, -halfLen, halfWid, halfLen);
        glColor3f(0, 0, 0);
        glLineWidth(1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(-halfWid, -halfLen); glVertex2f(halfWid, -halfLen);
        glVertex2f(halfWid, halfLen); glVertex2f(-halfWid, halfLen);
        glEnd();
        glPopMatrix();
    }
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
    float distanceAlongPath = 0.0f;

    glBegin(GL_LINES);
    for (int i = 0; i < shape.size() - 1; ++i) {
        QPointF p1 = shape[i];
        QPointF p2 = shape[i + 1];
        QVector2D dir(p2.x() - p1.x(), p2.y() - p1.y());
        float segmentLen = dir.length();
        dir.normalize();
        QVector2D normal(-dir.y(), dir.x());

        float currentPos = 0.0f;
        while (currentPos < segmentLen) {
            float relPos = fmod(distanceAlongPath + currentPos, totalPatternLen);
            if (relPos < dashLen) {
                float drawLen = std::min(dashLen - relPos, segmentLen - currentPos);
                QPointF v1(p1.x() + normal.x() * offsetValue + dir.x() * currentPos,
                    p1.y() + normal.y() * offsetValue + dir.y() * currentPos);
                QPointF v2(p1.x() + normal.x() * offsetValue + dir.x() * (currentPos + drawLen),
                    p1.y() + normal.y() * offsetValue + dir.y() * (currentPos + drawLen));
                glVertex2f(v1.x(), v1.y());
                glVertex2f(v2.x(), v2.y());
                currentPos += drawLen;
            }
            else {
                float skipLen = std::min(totalPatternLen - relPos, segmentLen - currentPos);
                currentPos += skipLen;
            }
        }
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
void MapWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        float rad = m_rotation * 3.1415926f / 180.0f;
        float dx = delta.x() * cos(-rad) - delta.y() * sin(-rad);
        float dy = delta.x() * sin(-rad) + delta.y() * cos(-rad);
        m_centerX -= dx / m_scale; m_centerY += dy / m_scale;
        update();
    }
}
void MapWidget::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0) m_scale *= 1.1f; else m_scale /= 1.1f;
    update();
}