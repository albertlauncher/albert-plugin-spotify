// Copyright (c) 2025-2025 Manuel Schneider

#include "configwidget.h"
#include "plugin.h"
#include <QStyle>
#include <albert/oauth.h>
#include <albert/oauthconfigwidget.h>
#include <albert/widgetsutil.h>
using namespace albert;
using namespace std;
using namespace util;


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

    using enum spotify::SearchTypeFlags;
    const initializer_list<pair<QCheckBox*, spotify::SearchTypeFlags>> checkBoxesTypes = {
        {ui.checkBox_album, Album},
        {ui.checkBox_artist, Artist},
        {ui.checkBox_audiobook, Audiobook},
        {ui.checkBox_episode, Episode},
        {ui.checkBox_show, Show},
        {ui.checkBox_track, Track},
        {ui.checkBox_playlist, Playlist}
    };

    for (const auto &[box, type] : checkBoxesTypes)
    {
        box->setText(spotify::localizedTypeString(type));
        box->setChecked(plugin.search_types() & type);
        connect(&plugin, &Plugin::search_types_changed, this, [=](spotify::SearchTypeFlags t){
            // QSignalBlocker b(box);
            box->setChecked(t & type);
        });
        connect(box, &QCheckBox::toggled, this,
                [=, this](bool b){
                    plugin.set_search_types((spotify::SearchTypeFlags)
                                    (b ? plugin.search_types() | type
                                       : plugin.search_types() & ~type));
                });
    }
}
