#include "MapData.h"

MapData::MapData() {
    // 初始化一个空的边界
    m_bounds = QRectF(0, 0, 100, 100);
}

void MapData::parseShape(const QString& shapeStr, QVector<QPointF>& outPoints)
{
    QStringList points = shapeStr.split(' ', Qt::SkipEmptyParts);
    for (const QString& ptStr : points) {
        QStringList coords = ptStr.split(',');
        if (coords.size() == 2) {
            outPoints.append(QPointF(coords[0].toFloat(), coords[1].toFloat()));
        }
    }
}

bool MapData::load(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Cannot open file" << filePath;
        return false;
    }

    QDomDocument doc;
    // Qt 6.5+ 推荐写法
    auto result = doc.setContent(&file);
    if (!result) {
        qDebug() << "XML Parse Error:" << result.errorMessage;
        file.close();
        return false;
    }
    file.close();

    m_edges.clear();
    m_junctions.clear();

    QDomElement root = doc.documentElement();
    QDomNode node = root.firstChild();

    float minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    bool hasData = false;

    while (!node.isNull()) {
        QDomElement element = node.toElement();
        if (!element.isNull()) {
            // 1. 解析 Edge
            if (element.tagName() == "edge") {
                Edge edge;
                edge.id = element.attribute("id");
                edge.function = element.attribute("function");

                QDomNode childNode = element.firstChild();
                while (!childNode.isNull()) {
                    QDomElement childElem = childNode.toElement();
                    if (childElem.tagName() == "lane") {
                        Lane lane;
                        lane.id = childElem.attribute("id");
                        lane.width = childElem.attribute("width", "3.0").toFloat();
                        parseShape(childElem.attribute("shape"), lane.shape);

                        for (const auto& pt : lane.shape) {
                            if (pt.x() < minX) minX = pt.x();
                            if (pt.x() > maxX) maxX = pt.x();
                            if (pt.y() < minY) minY = pt.y();
                            if (pt.y() > maxY) maxY = pt.y();
                            hasData = true;
                        }
                        edge.lanes.append(lane);
                    }
                    childNode = childNode.nextSibling();
                }
                m_edges.append(edge);
            }
            // 2. 解析 Junction
            else if (element.tagName() == "junction") {
                if (element.attribute("type") != "internal") {
                    Junction junc;
                    junc.id = element.attribute("id");
                    parseShape(element.attribute("shape"), junc.shape);
                    m_junctions.append(junc);
                }
            }
        }
        node = node.nextSibling();
    }

    if (hasData) {
        m_bounds = QRectF(minX, minY, maxX - minX, maxY - minY);
    }

    return true;
}