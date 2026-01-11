#pragma once
#include <QWidget>
#include <QTextEdit>

class InfoPanel : public QWidget {
    Q_OBJECT
public:
    explicit InfoPanel(QWidget* parent = nullptr);

public slots:
    // 提供一个槽函数来接收文本更新
    void updateInfo(const QString& info);

private:
    QTextEdit* m_textEdit;
};