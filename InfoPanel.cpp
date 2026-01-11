#include "InfoPanel.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QTime>

InfoPanel::InfoPanel(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* group = new QGroupBox("Global Text Scene");
    QVBoxLayout* layout = new QVBoxLayout(group);

    m_textEdit = new QTextEdit();
    m_textEdit->setReadOnly(true);
    m_textEdit->setText("System Ready...\nClick a vehicle to see details.");

    layout->addWidget(m_textEdit);
    mainLayout->addWidget(group);
}

void InfoPanel::updateInfo(const QString& info) {
    m_textEdit->setText(info);
    m_textEdit->append("\nQuery Time: " + QTime::currentTime().toString());
}