#include "MapRenderer.h"
#include <QImage>
#include <algorithm>
#include <cmath>
#include <QVector2D>
#include <QVector3D> // 用于更精确的计算

// 确保包含 GL 头文件
#include <GL/gl.h>

MapRenderer::MapRenderer() {
}

MapRenderer::~MapRenderer() {
    if (m_roadTexture) {
        delete m_roadTexture;
        m_roadTexture = nullptr;
    }
}

void MapRenderer::initialize() {
    initializeOpenGLFunctions();

    // 加载纹理 (确保 asphalt.jpg 在构建目录或工作目录下)
    QImage img("asphalt.jpg");
    if (!img.isNull()) {
        m_roadTexture = new QOpenGLTexture(img.mirrored());
        m_roadTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_roadTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_roadTexture->setWrapMode(QOpenGLTexture::Repeat);
    }
}

void MapRenderer::render(const std::shared_ptr<MapData>& data, const AppConfig& config, bool is3D, int style) {
    if (!data) return;

    // 1. 绘制路网
    if (style == 1) {
        drawRealisticRoads(data, config);
    }
    else {
        drawFlatRoads(data, config);
    }

    // 2. 绘制车辆
    drawVehicles(data, is3D);
}

void MapRenderer::drawVehicles(const std::shared_ptr<MapData>& data, bool is3D) {
    QString selectedId = data->getSelectedVehicleId();
    for (const auto& veh : data->vehicles()) {
        glPushMatrix();
        glTranslatef(veh.x, veh.y, 0.1f);
        glRotatef(-veh.angle, 0.0f, 0.0f, 1.0f);

        // 1. 正常绘制车辆
        if (is3D) {
            draw3DVehicle(veh.length, veh.width, veh.color);
        }
        else {
            // 2D 绘制
            glColor3f(veh.color.redF(), veh.color.greenF(), veh.color.blueF());
            float len = veh.length;
            float halfWid = veh.width / 2.0f;
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
        }

        // =========================================================
        // 高亮选中逻辑
        // =========================================================
        if (!selectedId.isEmpty() && veh.id == selectedId) {
            glDisable(GL_DEPTH_TEST); // 临时关闭深度测试，确保红框永远画在最上层，不被遮挡

            glColor3f(1.0f, 0.0f, 0.0f); // 纯红色
            glLineWidth(3.0f);           // 加粗线条

            float len = veh.length;
            float w = veh.width;
            float margin = 0.4f; // 框比车稍微大一点点

            // 绘制 3D 框 (线框盒) 或 2D 框
            if (is3D) {
                // 简单的 3D 包围盒
                float h = 2.0f; // 高度
                glBegin(GL_LINES);
                // 底面四边
                glVertex3f(-w / 2 - margin, -len - margin, 0); glVertex3f(w / 2 + margin, -len - margin, 0);
                glVertex3f(w / 2 + margin, -len - margin, 0);  glVertex3f(w / 2 + margin, margin, 0);
                glVertex3f(w / 2 + margin, margin, 0);         glVertex3f(-w / 2 - margin, margin, 0);
                glVertex3f(-w / 2 - margin, margin, 0);        glVertex3f(-w / 2 - margin, -len - margin, 0);
                // 顶面四边
                glVertex3f(-w / 2 - margin, -len - margin, h); glVertex3f(w / 2 + margin, -len - margin, h);
                glVertex3f(w / 2 + margin, -len - margin, h);  glVertex3f(w / 2 + margin, margin, h);
                glVertex3f(w / 2 + margin, margin, h);         glVertex3f(-w / 2 - margin, margin, h);
                glVertex3f(-w / 2 - margin, margin, h);        glVertex3f(-w / 2 - margin, -len - margin, h);
                // 立柱
                glVertex3f(-w / 2 - margin, -len - margin, 0); glVertex3f(-w / 2 - margin, -len - margin, h);
                glVertex3f(w / 2 + margin, -len - margin, 0);  glVertex3f(w / 2 + margin, -len - margin, h);
                glVertex3f(w / 2 + margin, margin, 0);         glVertex3f(w / 2 + margin, margin, h);
                glVertex3f(-w / 2 - margin, margin, 0);        glVertex3f(-w / 2 - margin, margin, h);
                glEnd();
            }
            else {
                // 2D 红框
                glBegin(GL_LINE_LOOP);
                glVertex2f(-w / 2 - margin, -len - margin);
                glVertex2f(w / 2 + margin, -len - margin);
                glVertex2f(w / 2 + margin, 0.0f + margin);
                glVertex2f(-w / 2 - margin, 0.0f + margin);
                glEnd();
            }

            glLineWidth(1.0f); // 恢复线宽
            glEnable(GL_DEPTH_TEST); // 恢复深度测试
        }

        glPopMatrix();
    }
}

void MapRenderer::draw3DVehicle(float length, float width, QColor color) {
    float h = 1.6f;
    float halfW = width / 2.0f;

    // 车顶
    glColor3f(color.redF(), color.greenF(), color.blueF());
    glBegin(GL_QUADS);
    glVertex3f(-halfW, 0.0f, h);    glVertex3f(halfW, 0.0f, h);
    glVertex3f(halfW, -length, h);  glVertex3f(-halfW, -length, h);
    glEnd();

    // 侧面 (调暗)
    QColor sideColor = color.darker(115);
    glColor3f(sideColor.redF(), sideColor.greenF(), sideColor.blueF());
    glBegin(GL_QUADS);
    // 左
    glVertex3f(-halfW, 0.0f, h); glVertex3f(-halfW, -length, h); glVertex3f(-halfW, -length, 0.0f); glVertex3f(-halfW, 0.0f, 0.0f);
    // 右
    glVertex3f(halfW, 0.0f, h); glVertex3f(halfW, 0.0f, 0.0f); glVertex3f(halfW, -length, 0.0f); glVertex3f(halfW, -length, h);
    glEnd();

    // 车尾
    QColor backColor = color.darker(130);
    glColor3f(backColor.redF(), backColor.greenF(), backColor.blueF());
    glBegin(GL_QUADS);
    glVertex3f(-halfW, -length, h); glVertex3f(halfW, -length, h); glVertex3f(halfW, -length, 0.0f); glVertex3f(-halfW, -length, 0.0f);
    glEnd();

    // 车头 (挡风玻璃)
    glColor3f(sideColor.redF(), sideColor.greenF(), sideColor.blueF());
    glBegin(GL_QUADS);
    glVertex3f(-halfW, 0.0f, h * 0.4f); glVertex3f(halfW, 0.0f, h * 0.4f); glVertex3f(halfW, 0.0f, 0.0f); glVertex3f(-halfW, 0.0f, 0.0f);
    glEnd();

    glColor3f(0.7f, 0.8f, 0.9f);
    glBegin(GL_QUADS);
    glVertex3f(-halfW, 0.0f, h); glVertex3f(halfW, 0.0f, h); glVertex3f(halfW, 0.0f, h * 0.4f); glVertex3f(-halfW, 0.0f, h * 0.4f);
    glEnd();
}

void MapRenderer::drawRealisticRoads(const std::shared_ptr<MapData>& data, const AppConfig& config) {
    // 1. 绘制纹理路口
    glEnable(GL_TEXTURE_2D);
    if (m_roadTexture) m_roadTexture->bind();
    glColor3f(0.25f, 0.25f, 0.25f);
    for (const auto& junc : data->junctions()) {
        if (!junc.shape.empty()) {
            glBegin(GL_POLYGON);
            for (const auto& pt : junc.shape) {
                glTexCoord2f(pt.x() * 0.2f, pt.y() * 0.2f); glVertex2f(pt.x(), pt.y());
            }
            glEnd();
        }
    }
    // 2. 绘制纹理车道
    for (const auto& edge : data->edges()) {
        if (edge.function == "internal") continue;
        for (const auto& lane : edge.lanes) {
            drawWideLane(lane.shape, lane.width, QColor::fromRgbF(0.25f, 0.25f, 0.25f));
        }
    }
    if (m_roadTexture) m_roadTexture->release();
    glDisable(GL_TEXTURE_2D);

    // 3. 绘制标线
    for (const auto& edge : data->edges()) {
        if (edge.function == "internal") continue;
        int laneCount = edge.lanes.size();
        for (int i = 0; i < laneCount; ++i) {
            const Lane& lane = edge.lanes[i];
            float hW = lane.width / 2.0f;

            // 绘制车道左侧的线（如果是多车道）
            if (i < laneCount - 1) {
                if (i == 2) { // 示例：黄色实线
                    glColor3f(1.0f, 1.0f, 0.0f); glDisable(GL_LINE_STIPPLE); glLineWidth(3.5f);
                    renderSingleLine(lane.shape, hW);
                }
                else { // 虚线
                    glColor3f(1.0f, 1.0f, 1.0f); glDisable(GL_LINE_STIPPLE); glLineWidth(1.5f);
                    renderGeometricDashedLine(lane.shape, hW, 4.0f, 6.0f);
                }
            }

            // 【修复问题1】必须分开判断，确保单车道时左右两条线都会画
            glColor3f(0.85f, 0.85f, 0.85f); glDisable(GL_LINE_STIPPLE); glLineWidth(2.0f);

            // 如果是第0条车道（最右侧），画右边线 (-hW)
            if (i == 0) {
                renderSingleLine(lane.shape, -hW);
            }
            // 如果是最后一条车道（最左侧），画左边线 (hW)
            if (i == laneCount - 1) {
                renderSingleLine(lane.shape, hW);
            }

            // 绘制停止线
            drawStopLine(data, lane.shape, lane.width, true);
            drawStopLine(data, lane.shape, lane.width, false);
        }
    }
    glLineWidth(1.0f);
}

void MapRenderer::drawFlatRoads(const std::shared_ptr<MapData>& data, const AppConfig& config) {
    QColor roadColor = config.map.roadColor;
    glColor3f(roadColor.redF(), roadColor.greenF(), roadColor.blueF());
    glLineWidth(config.map.roadLineWidth);

    for (const auto& edge : data->edges()) {
        for (const auto& lane : edge.lanes) {
            glBegin(GL_LINE_STRIP);
            for (const auto& pt : lane.shape) glVertex2f(pt.x(), pt.y());
            glEnd();
        }
    }
}

// --- 辅助绘图函数 ---

void MapRenderer::drawWideLane(const QVector<QPointF>& shape, float width, QColor color) {
    if (shape.size() < 2) return;
    float halfW = width / 2.0f;
    glColor3f(color.redF(), color.greenF(), color.blueF());
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i < shape.size(); ++i) {
        QVector2D normal;
        QPointF p = shape[i];
        if (i < shape.size() - 1) {
            QVector2D dir(shape[i + 1].x() - p.x(), shape[i + 1].y() - p.y());
            dir.normalize(); normal = QVector2D(-dir.y(), dir.x());
        }
        else {
            QVector2D dir(p.x() - shape[i - 1].x(), p.y() - shape[i - 1].y());
            dir.normalize(); normal = QVector2D(-dir.y(), dir.x());
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

void MapRenderer::renderSingleLine(const QVector<QPointF>& shape, float offsetValue) {
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
        // 车道线高度 Z=0.02
        glVertex3f(pt.x(), pt.y(), 0.02f);
    }
    glEnd();
}

void MapRenderer::renderGeometricDashedLine(const QVector<QPointF>& shape, float offsetValue, float dashLen, float gapLen) {
    if (shape.size() < 2) return;
    float totalPatternLen = dashLen + gapLen;
    QVector2D dir(shape[1].x() - shape[0].x(), shape[1].y() - shape[0].y());
    if (dir.lengthSquared() > 0.0001f) dir.normalize(); else dir = QVector2D(1.0f, 0.0f);

    float worldProjection = shape[0].x() * dir.x() + shape[0].y() * dir.y();
    float distanceAlongPath = fmod(worldProjection, totalPatternLen);
    if (distanceAlongPath < 0.0f) distanceAlongPath += totalPatternLen;

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
            float relPos = fmod(distanceAlongPath + currentPos, totalPatternLen);
            if (relPos < dashLen) {
                float drawLen = std::min(dashLen - relPos, segmentLen - currentPos);
                QPointF v1(p1.x() + normal.x() * offsetValue + dirVec.x() * currentPos,
                    p1.y() + normal.y() * offsetValue + dirVec.y() * currentPos);
                QPointF v2(p1.x() + normal.x() * offsetValue + dirVec.x() * (currentPos + drawLen),
                    p1.y() + normal.y() * offsetValue + dirVec.y() * (currentPos + drawLen));
                // 虚线高度 Z=0.02
                glVertex3f(v1.x(), v1.y(), 0.02f);
                glVertex3f(v2.x(), v2.y(), 0.02f);
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

// 【修复问题2】修复夹角问题，使用 double 精度计算向量，确保垂直
void MapRenderer::drawStopLine(const std::shared_ptr<MapData>& data, const QVector<QPointF>& shape, float laneWidth, bool isEnd) {
    if (shape.size() < 2 || !data) return;
    QPointF basePt = isEnd ? shape.last() : shape.first();
    QRectF bounds = data->bounds();

    // 简单边界检查，防止画在地图外
    float eps = 1.5f;
    if (basePt.x() <= bounds.left() + eps || basePt.x() >= bounds.right() - eps ||
        basePt.y() <= bounds.top() + eps || basePt.y() >= bounds.bottom() - eps) return;

    // 使用 double 精度 (QPointF) 进行减法，避免 float 大坐标相减造成精度丢失导致方向歪斜
    double dx, dy;
    if (isEnd) {
        dx = basePt.x() - shape[shape.size() - 2].x();
        dy = basePt.y() - shape[shape.size() - 2].y();
    }
    else {
        dx = shape[1].x() - basePt.x();
        dy = shape[1].y() - basePt.y();
    }

    // 归一化
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.0001) return;
    dx /= len; dy /= len;

    // roadDir 是道路纵向 (double)
    // normal 是道路横向 (-dy, dx)

    float halfW = laneWidth / 2.0f;
    float stripeLen = 1.2f;
    int stripeCount = 3;

    glDisable(GL_LINE_STIPPLE);
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.5f);
    glBegin(GL_LINES);

    for (int i = 0; i <= stripeCount; ++i) {
        // t 从 -1 到 1，覆盖车道宽度
        float t = -1.0f + 2.0f * i / stripeCount;

        // pS: 停止线的起始点 (横向分布)
        // 使用 double 计算后转 float
        float psX = (float)(basePt.x() + (-dy) * halfW * t);
        float psY = (float)(basePt.y() + (dx)*halfW * t);

        // pE: 停止线的结束点 (沿道路方向延伸)
        float sign = isEnd ? -1.0f : 1.0f;
        float peX = (float)(psX + dx * stripeLen * sign);
        float peY = (float)(psY + dy * stripeLen * sign);

        // 高度 Z=0.02
        glVertex3f(psX, psY, 0.02f);
        glVertex3f(peX, peY, 0.02f);
    }
    glEnd();
}