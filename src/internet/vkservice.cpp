#include <QMenu>
#include <QSettings>
#include <QByteArray>

#include <boost/scoped_ptr.hpp>

#include "internetmodel.h"
#include "internetplaylistitem.h"
#include "core/application.h"
#include "core/logging.h"
#include "ui/iconloader.h"

#include "vkservice.h"
#include "vreen/auth/oauthconnection.h"

#define  __(var) qLog(Debug) << #var " =" << (var);

const char* VkService::kServiceName = "Vk.com";
const char* VkService::kSettingGroup = "Vk.com";
const uint  VkService::kApiKey = 3421812;

VkService::VkService(Application *app, InternetModel *parent) :
    InternetService(kServiceName, app, parent, parent),
    need_login_(NULL),
    root_item_(NULL),
    recommendations_(NULL),
    my_music_(NULL),
    context_menu_(new QMenu),
    client_(new Vreen::Client)
{
    QSettings s;
    s.beginGroup(kSettingGroup);

    /* Init connection */
    QByteArray token = s.value("token",QByteArray()).toByteArray();
    uint uid = s.value("uid",0).toUInt();
    hasAccount_ = not (!uid or token.isEmpty());
    __(token)

    connection_ = new Vreen::OAuthConnection(kApiKey,client_);
    connection_->setConnectionOption(Vreen::Connection::ShowAuthDialog,true);
    client_->setConnection(connection_);
    if (hasAccount_) {
         qLog(Debug) << "--- Have account";
        time_t expiresIn = s.value("expiresIn", 0).toUInt();
        uint uid = s.value("uid",0).toUInt();
        __(expiresIn)
        connection_->setAccessToken(token, expiresIn);
        connection_->setUid(uid);
        Login();
    }

    connect(connection_, SIGNAL(accessTokenChanged(QByteArray,time_t)),
            SLOT(ChangeAccessToken(QByteArray,time_t)));
    connect(client_, SIGNAL(onlineStateChanged(bool)),
            SLOT(OnlineStateChanged(bool)));

    /* Init interface */
    context_menu_->addActions(GetPlaylistActions());
    context_menu_->addAction(IconLoader::Load("configure"), tr("Configure Vk.com..."),
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
        RefreshRootSubitems();
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

void VkService::RefreshRootSubitems()
{
    if (root_item_->hasChildren()){
        root_item_->removeRows(0, root_item_->rowCount());
    }

    recommendations_ = NULL;
    my_music_ = NULL;
    need_login_ = NULL;

    if (hasAccount_) {

        recommendations_ = new QStandardItem(
                    QIcon(":vk/recommends.png"),
                    tr("My Recommendations"));
        recommendations_->setData(Type_Recommendations, InternetModel::Role_Type);
        root_item_->appendRow(recommendations_);

        my_music_ = new QStandardItem(
                    QIcon(":vk/my_music.png"),
                    tr("My Music"));
        my_music_->setData(Type_MyMusic, InternetModel::Role_CanLazyLoad);
        root_item_->appendRow(my_music_);
    } else {
        need_login_ = new QStandardItem(
                    QIcon(),
                    tr("Double click to login")
                    );
        need_login_->setData(Type_NeedLogin, InternetModel::Role_Type);
        need_login_->setData(InternetModel::PlayBehaviour_DoubleClickAction,
                             InternetModel::Role_PlayBehaviour);
        root_item_->appendRow(need_login_);
    }
}

void VkService::Login()
{
    qLog(Debug) << "--- Login";
    if (hasAccount_) {
        emit LoginSuccess(true);
    }
    client_->connectToHost();
}

void VkService::Logout()
{
    qLog(Debug) << "--- Logout";
    client_->disconnectFromHost();

    hasAccount_ = false;
    RefreshRootSubitems();

    QSettings s;
    s.beginGroup(kSettingGroup);
    s.setValue("token", QByteArray());
    s.setValue("expiresIn",0);
    s.setValue("uid",uint(0));
    connection_->clear();
}

void VkService::ShowConfig()
{
    app_->OpenSettingsDialogAtPage(SettingsDialog::Page_Vk);
}

void VkService::ChangeAccessToken(const QByteArray &token, time_t expiresIn)
{
    qLog(Debug) << "--- Access token changed";
    QSettings s;
    s.beginGroup(kSettingGroup);
    s.setValue("token", token);
    s.setValue("expiresIn",uint(expiresIn));
    s.setValue("uid",uint(connection_->uid()));
}

void VkService::OnlineStateChanged(bool online)
{
    qLog(Debug) << "--- Online state changed to" << online;
    if (online) {
        hasAccount_ = true;
        emit LoginSuccess(true);
        RefreshRootSubitems();
    }
}
