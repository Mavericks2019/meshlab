// app_ui.h
#ifndef APP_UI_H
#define APP_UI_H

#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QColorDialog>
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QFont>
#include <QVBoxLayout>
#include <QObject>

namespace AppUI {
    
    // 创建模型信息显示组
    QGroupBox* createModelInfoGroup(QLabel** infoLabel = nullptr);
    
    // 创建颜色设置组
    QGroupBox* createColorSettingsGroup(QWidget* glWidget);
    
    // 应用深色主题
    void applyDarkTheme(QApplication& app);
    
} // namespace AppUI

#endif // APP_UI_H