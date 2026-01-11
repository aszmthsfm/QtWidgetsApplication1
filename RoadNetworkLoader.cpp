#include "RoadNetworkLoader.h"
#include "MapData.h" 
#include <QFile>
#include <QDomDocument>
#include <QtMath>
#include <QDebug>

bool RoadNetworkLoader::load(const QString& filePath, std::shared_ptr<MapData> targetData) {
    if (!targetData) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Cannot open road network file:" << filePath;
        return false;
    }

    QDomDocument doc;
    if (!doc.setContent(&file)) {
        file.close();
        qDebug() << "Error: Failed to parse XML content.";
        return false;
    }
    file.close();

    // Çå¿Õ¾ÉÊý¾Ý
    targetData->clear();

    QDomElement root = doc.documentElement();
    QDomNode node = root.firstChild();

    float minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    bool hasData = false;

    while (!node.isNull()) {
        QDomElement element = node.toElement();
        if (!element.isNull() && element.tagName() == "edge") {
            Edge edge;
            edge.id = element.attribute("id");
            edge.fromJunc = element.attribute("from");
            edge.toJunc = element.attribute("to");
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
                        minX = std::min((float)pt.x(), minX); maxX = std::max((float)pt.x(), maxX);
                        minY = std::min((float)pt.y(), minY); maxY = std::max((float)pt.y(), maxY);
                        hasData = true;
                    }
                    edge.lanes.append(lane);
                }
                childNode = childNode.nextSibling();
            }
            targetData->addEdge(edge);
        }
        else if (!element.isNull() && element.tagName() == "junction" && element.attribute("type") != "internal") {
            Junction junc;
            junc.id = element.attribute("id");
            parseShape(element.attribute("shape"), junc.shape);
            targetData->addJunction(junc);
        }
        node = node.nextSibling();
    }

    if (hasData) {
        targetData->setBounds(QRectF(minX, minY, maxX - minX, maxY - minY));
    }

    qDebug() << "RoadNetworkLoader: Loaded" << targetData->edges().size() << "edges and"
        << targetData->junctions().size() << "junctions.";

    return true;
}

void RoadNetworkLoader::parseShape(const QString& shapeStr, QVector<QPointF>& outPoints) {
    QStringList points = shapeStr.split(' ', Qt::SkipEmptyParts);
    for (const QString& ptStr : points) {
        QStringList coords = ptStr.split(',');
        if (coords.size() == 2) {
            outPoints.append(QPointF(coords[0].toFloat(), coords[1].toFloat()));
        }
    }
}