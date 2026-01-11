#pragma once
#include <QString>
#include <QPointF>
#include <QVector>
#include <memory>

// 前置声明
class MapData;

class RoadNetworkLoader {
public:
    // 静态函数：解析 xml 文件，并将数据填充到 mapData 中
    static bool load(const QString& filePath, std::shared_ptr<MapData> targetData);

private:
    // 内部辅助函数
    static void parseShape(const QString& shapeStr, QVector<QPointF>& outPoints);
};