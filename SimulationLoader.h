#pragma once
#include <QString>
#include <QVector>
#include <QStringList>
#include "Types.h" 

class SimulationLoader {
public:
    SimulationLoader();

    // 初始化：扫描目录下的所有 JSON 文件
    bool init(const QString& directoryPath);

    // 读取指定帧的数据
    // 返回值：当前帧所有车辆的列表（原始数据）
    QVector<Vehicle> loadFrame(int frameIndex);

    // 获取总帧数
    int frameCount() const { return m_jsonFiles.size(); }

private:
    QString m_dataDir;
    QStringList m_jsonFiles;
};