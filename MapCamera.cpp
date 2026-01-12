#include "MapCamera.h"
#include <QOpenGLFunctions> // 为了使用 GL_PROJECTION 等宏，或者直接包含 <GL/gl.h>
#include <QtMath>
#include <cmath>
#include <algorithm>

// 如果你的环境没有自动包含 OpenGL 头文件，可能需要：
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h> 

MapCamera::MapCamera() {
    // 初始化默认值
}

void MapCamera::setViewport(int w, int h) {
    m_viewportW = w;
    m_viewportH = h;
}

void MapCamera::setFocus(float x, float y, float zoomVal) {
    m_centerX = x;
    m_centerY = y;
    m_scale = zoomVal;
}

void MapCamera::set3D(bool enable) {
    m_is3D = enable;
    if (m_is3D) {
        m_pitch = 45.0f;
        m_cameraZ = 200.0f;
    }
    else {
        m_rotation = 0.0f;
    }
}

void MapCamera::setRotation(float angle) {
    m_rotation = angle;
}

void MapCamera::applyProjection() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (m_is3D) {
        setPerspectiveProjection();
    }
    else {
        if (m_scale <= 0.001f) m_scale = 0.001f;
        float viewHalfW = (m_viewportW / m_scale) / 2.0f;
        float viewHalfH = (m_viewportH / m_scale) / 2.0f;
        glOrtho(-viewHalfW, viewHalfW, -viewHalfH, viewHalfH, -2000.0, 2000.0);
    }
}

void MapCamera::setPerspectiveProjection() {
    if (m_viewportH == 0) m_viewportH = 1;
    float aspectRatio = (float)m_viewportW / (float)m_viewportH;
    float fov = 45.0f;
    float zNear = 1.0f;
    float zFar = 10000.0f;

    float top = zNear * std::tan(fov * 0.5f * 3.1415926f / 180.0f);
    float bottom = -top;
    float right = top * aspectRatio;
    float left = -right;

    glFrustum(left, right, bottom, top, zNear, zFar);
}

QMatrix4x4 MapCamera::getProjectionMatrix() {
    QMatrix4x4 proj;
    proj.setToIdentity();
    if (m_is3D) {
        float aspectRatio = (float)m_viewportW / (m_viewportH ? m_viewportH : 1);
        proj.perspective(45.0f, aspectRatio, 1.0f, 10000.0f);
    }
    else {
        if (m_scale <= 0.001f) m_scale = 0.001f;
        float w = (m_viewportW / m_scale) / 2.0f;
        float h = (m_viewportH / m_scale) / 2.0f;
        proj.ortho(-w, w, -h, h, -2000.0f, 2000.0f);
    }
    return proj;
}

QMatrix4x4 MapCamera::getViewMatrix() {
    QMatrix4x4 view;
    view.setToIdentity();
    if (m_is3D) {
        view.translate(0.0f, 0.0f, -m_cameraZ);
        view.rotate(-m_pitch, 1.0f, 0.0f, 0.0f);
        view.rotate(m_rotation, 0.0f, 0.0f, 1.0f);
        view.translate(-m_centerX, -m_centerY, 0.0f);
    }
    else {
        view.rotate(m_rotation, 0.0f, 0.0f, 1.0f);
        view.translate(-m_centerX, -m_centerY, 0.0f);
    }
    return view;
}

Ray MapCamera::getRay(const QPoint& screenPos) {
    QMatrix4x4 proj = getProjectionMatrix();
    QMatrix4x4 view = getViewMatrix();
    QMatrix4x4 invVP = (proj * view).inverted();

    // 屏幕坐标转 NDC (Normalized Device Coordinates)
    // 注意：Qt 的屏幕原点在左上角，OpenGL 在左下角，需要翻转 Y
    float x = (2.0f * screenPos.x()) / m_viewportW - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / m_viewportH;

    // 1. 近平面点 (NDS Z = -1.0)
    QVector4D nearPoint(x, y, -1.0f, 1.0f);
    QVector4D nearWorld = invVP * nearPoint;
    nearWorld /= nearWorld.w();

    // 2. 远平面点 (NDS Z = 1.0)
    QVector4D farPoint(x, y, 1.0f, 1.0f);
    QVector4D farWorld = invVP * farPoint;
    farWorld /= farWorld.w();

    Ray ray;
    ray.origin = nearWorld.toVector3D();
    ray.direction = (farWorld - nearWorld).toVector3D().normalized();
    return ray;
}

void MapCamera::applyModelView() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (m_is3D) {
        glTranslatef(0.0f, 0.0f, -m_cameraZ);
        glRotatef(-m_pitch, 1.0f, 0.0f, 0.0f);
        glRotatef(m_rotation, 0.0f, 0.0f, 1.0f);
        glTranslatef(-m_centerX, -m_centerY, 0.0f);
    }
    else {
        glRotatef(m_rotation, 0.0f, 0.0f, 1.0f);
        glTranslatef(-m_centerX, -m_centerY, 0.0f);
    }
}

void MapCamera::pan(const QPoint& delta) {
    float rad = m_rotation * M_PI / 180.0f;
    float cosA = cos(-rad);
    float sinA = sin(-rad);
    float dx = delta.x();
    float dy = delta.y();

    // 3D 模式下根据高度调整移动速度
    float panScale = m_is3D ? (m_cameraZ / 600.0f) : (1.0f / m_scale);

    float moveX = dx * cosA - dy * sinA;
    float moveY = dx * sinA + dy * cosA;

    m_centerX -= moveX * panScale;
    m_centerY += moveY * panScale;
}

void MapCamera::rotate(const QPoint& delta) {
    if (m_is3D) {
        m_rotation += delta.x() * 0.5f;
        m_pitch += delta.y() * 0.5f;
        if (m_pitch < 10.0f) m_pitch = 10.0f;
        if (m_pitch > 89.0f) m_pitch = 89.0f;
    }
    else {
        m_rotation += delta.x() * 0.5f;
    }
}

void MapCamera::zoom(int angleDelta) {
    if (m_is3D) {
        if (angleDelta > 0) m_cameraZ *= 0.9f;
        else m_cameraZ *= 1.1f;

        if (m_cameraZ < 10.0f) m_cameraZ = 10.0f;
        if (m_cameraZ > 5000.0f) m_cameraZ = 5000.0f;
    }
    else {
        if (angleDelta > 0) m_scale *= 1.1f;
        else m_scale /= 1.1f;
    }
}

QPointF MapCamera::screenToWorld(const QPoint& screenPos) {
    float x = screenPos.x();
    float y = screenPos.y();

    if (!m_is3D) {
        float ndcX = (x - m_viewportW / 2.0f);
        float ndcY = (m_viewportH / 2.0f - y);
        float worldX = ndcX / m_scale;
        float worldY = ndcY / m_scale;

        float rad = -m_rotation * M_PI / 180.0f;
        float rotX = worldX * cos(rad) - worldY * sin(rad);
        float rotY = worldX * sin(rad) + worldY * cos(rad);

        return QPointF(rotX + m_centerX, rotY + m_centerY);
    }
    else {
        // 3D 射线拾取逻辑
        float ndcX = (2.0f * x) / m_viewportW - 1.0f;
        float ndcY = 1.0f - (2.0f * y) / m_viewportH;

        float fov = 45.0f;
        float aspect = (float)m_viewportW / m_viewportH;
        float tanHalfFov = tan(qDegreesToRadians(fov / 2.0f));

        float viewX = ndcX * aspect * tanHalfFov;
        float viewY = ndcY * tanHalfFov;
        float viewZ = -1.0f;

        // 简化的射线向量
        float rayX = viewX;
        float rayY = viewY;
        float rayZ = viewZ;

        // 归一化
        float len = std::sqrt(rayX * rayX + rayY * rayY + rayZ * rayZ);
        rayX /= len; rayY /= len; rayZ /= len;

        // 1. 逆俯仰角 (Pitch)
        float pitchRad = qDegreesToRadians(m_pitch);
        float y1 = rayY * cos(pitchRad) - rayZ * sin(pitchRad);
        float z1 = rayY * sin(pitchRad) + rayZ * cos(pitchRad);
        rayY = y1; rayZ = z1;

        // 2. 逆旋转 (Rotation)
        float rotRad = qDegreesToRadians(-m_rotation);
        float x2 = rayX * cos(rotRad) - rayY * sin(rotRad);
        float y2 = rayX * sin(rotRad) + rayY * cos(rotRad);
        rayX = x2; rayY = y2;

        if (std::abs(rayZ) < 0.0001f) return QPointF(0, 0);

        float t = -m_cameraZ / rayZ;
        return QPointF(m_centerX + t * rayX, m_centerY + t * rayY);
    }
}

void MapCamera::resetCameraAngles() {
    if (m_is3D) {
        // 3D 默认参数
        m_pitch = 45.0f;       // 默认俯仰角
        m_rotation = 0.0f;     // 默认朝向正北
        m_cameraZ = 200.0f;    // 默认高度
    }
    else {
        // 2D 默认参数
        m_rotation = 0.0f;     // 默认朝向正北
    }
}