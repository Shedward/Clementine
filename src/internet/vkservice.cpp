#include <QMenu>
#include <QSettings>

#include <boost/scoped_ptr.hpp>

#include "internetmodel.h"
#include "internetplaylistitem.h"
#include "core/application.h"
#include "core/logging.h"
#include "ui/iconloader.h"

#include "vkservice.h"
#include "vreen/auth/oauthconnection.h"

const char* VkService::kServiceName = "Vk.com";
const char* VkService::kSettingGroup = "Vk.com";
const uint  VkService::kApiKey = 3421812;

VkService::VkService(Application *app, InternetModel *parent) :
    InternetService(kServiceName, app, parent, parent),
    context_menu_(new QMenu)
{
    auto *auth = new Vreen::OAuthConnection(kApiKey,&client_);
    auth->setConnectionOption(Vreen::Connection::KeepAuthData, true);
    auth->setConnectionOption(Vreen::Connection::ShowAuthDialog,true);
    client_.setConnection(auth);

    QSettings s;
    s.beginGroup(kSettingGroup);
    connected_ = not s.value("name").toString().isEmpty();

    context_menu_->addActions(GetPlaylistActions());
    context_menu_->addAction(IconLoader::Load("configure"), tr("Configure Last.fm..."),
                             this, SLOT(ShowConfig()));

}

VkService::~VkService()
{
}


QStandardItem *VkService::CreateRootItem()
{
    root_item_ = new QStandardItem(QIcon(":providers/vk.png"),kServiceName);
    root_item_->setData(true, InternetModel::Role_CanLazyLoad);
    return root_item_;
}

void VkService::LazyPopulate(QStandardItem *parent)
{
    switch (parent->data(InternetModel::Role_Type).toInt()) {
    case InternetModel::Type_Service:
        if (connected_) {
            recommendations_ = new QStandardItem(
                        QIcon(":vk/recommends.png"),
                        tr("My Recommendations"));
            recommendations_->setData(Type_Recommendations, InternetModel::Role_Type);
            parent->appendRow(recommendations_);

            my_music_ = new QStandardItem(
                        QIcon(":vk/my_music.png"),
                        tr("My Music"));
            my_music_->setData(Type_MyMusic, InternetModel::Role_CanLazyLoad);
            parent->appendRow(my_music_);
        } else {
            need_login_ = new QStandardItem(
                        QIcon(),
                        tr("Double click to login")
                        );
            need_login_->setData(Type_NeedLogin, InternetModel::Role_Type);
            need_login_->setData(InternetModel::PlayBehaviour_DoubleClickAction,
                                 InternetModel::Role_PlayBehaviour);
            parent->appendRow(need_login_);
        }
        break;
    default:
        break;
    }
}

void VkService::ShowContextMenu(const QPoint &global_pos)
{
    const bool playable = model()->IsPlayable(model()->current_index());
    GetAppendToPlaylistAction()->setEnabled(playable);
    GetReplacePlaylistAction()->setEnabled(playable);
    GetOpenInNewPlaylistAction()->setEnabled(playable);
    context_menu_->popup(global_pos);
}

void VkService::ItemDoubleClicked(QStandardItem *item)
{
    if (item == need_login_) {
        ShowConfig();
    }
}

void VkService::Login()
{
}

void VkService::Logout()
{
}


void VkService::ShowConfig()
{
    app_->OpenSettingsDialogAtPage(SettingsDialog::Page_Vk);
}
