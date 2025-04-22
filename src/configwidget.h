// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include <QWidget>
#include "ui_configwidget.h"
class Plugin;

class ConfigWidget final : public QWidget
{
    Q_OBJECT

public:

    explicit ConfigWidget(Plugin &plugin);

private:

    Ui::ConfigWidget ui;
    Plugin &plugin;

};
