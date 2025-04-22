// Copyright (c) 2025-2025 Manuel Schneider

#include "configwidget.h"
#include "plugin.h"
#include <albert/oauthconfigwidget.h>
#include <albert/widgetsutil.h>
using namespace albert::util;
using namespace albert;
using namespace std;


ConfigWidget::ConfigWidget(Plugin &p):
    plugin(p)
{
    ui.setupUi(this);

    auto oaw = new OAuthConfigWidget(plugin.oauth);
    ui.groupBox_oauth->layout()->addWidget(oaw);

    albert::util::bind(ui.checkBox_explicit,
                       &plugin,
                       &Plugin::show_explicit_content,
                       &Plugin::set_show_explicit_content,
                       &Plugin::show_explicit_content_changed);
}
