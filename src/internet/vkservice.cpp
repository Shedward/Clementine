#include <math.h>

#include <QMenu>
#include <QSettings>
#include <QByteArray>

#include <boost/scoped_ptr.hpp>
#include <algorithm>

#include "core/application.h"
#include "core/closure.h"
#include "core/logging.h"
#include "core/timeconstants.h"
#include "ui/iconloader.h"

#include "internetmodel.h"
#include "internetplaylistitem.h"
#include "globalsearch/globalsearch.h"

#include "vreen/auth/oauthconnection.h"
#include "vreen/audio.h"
#include "vreen/contact.h"
#include "vreen/roster.h"

#include "globalsearch/vksearchprovider.h"
#include "vkservice.h"

const char*  VkService::kServiceName = "Vk.com";
const char*  VkService::kSettingGroup = "Vk.com";
const uint   VkService::kApiKey = 3421812;
const Scopes VkService::kScopes =
        Vreen::OAuthConnection::Offline |
        Vreen::OAuthConnection::Audio |
        Vreen::OAuthConnection::Friends |
        Vreen::OAuthConnection::Groups;


VkService::VkService(Application *app, InternetModel *parent) :
    InternetService(kServiceName, app, parent, parent),
    need_login_(NULL),
    root_item_(NULL),
    recommendations_(NULL),
    my_music_(NULL),
    context_menu_(new QMenu),
    client_(new Vreen::Client),
    connection_(NULL),
    hasAccount_(false),
    provider_(NULL),
    last_id_(0)
{
    QSettings s;
    s.beginGroup(kSettingGroup);

    /* Init connection */
    provider_ = new Vreen::AudioProvider(client_);

    client_->setTrackMessages(false);
    client_->setInvisible(true);

    QByteArray token = s.value("token",QByteArray()).toByteArray();
    int uid = s.value("uid",0).toInt();
    hasAccount_ = not (!uid or token.isEmpty());

    if (hasAccount_) {
        Login();
    };

    connect(client_, SIGNAL(onlineStateChanged(bool)),
            SLOT(OnlineStateChanged(bool)));
    connect(client_, SIGNAL(error(Vreen::Client::Error)),
            SLOT(Error(Vreen::Client::Error)));

    /* Init interface */
    context_menu_->addActions(GetPlaylistActions());
    context_menu_->addAction(IconLoader::Load("configure"), tr("Configure Vk.com..."),
                             this, SLOT(ShowConfig()));

    VkSearchProvider* search_provider = new VkSearchProvider(app_, this);
    search_provider->Init(this);
    app_->global_search()->AddProvider(search_provider);
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
    case Type_MyMusic: {
        qDebug() << "Load My Music";
        UpdateMyMusic();
    }
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
    ClearStandartItem(root_item_);

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
        my_music_->setData(Type_MyMusic, InternetModel::Role_Type);
        my_music_->setData(true, InternetModel::Role_CanLazyLoad);
        my_music_->setData(InternetModel::PlayBehaviour_MultipleItems,
                           InternetModel::Role_PlayBehaviour);
        root_item_->appendRow(my_music_);

        loading_ = new QStandardItem(
                    QIcon(),
                    tr("Loading...")
                    );
        loading_->setData(Type_Loading, InternetModel::Role_Type);
        qDebug() << "size" << sizeof(*loading_);
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
    if (connection_) {
        client_->connectToHost();
        emit LoginSuccess(true);
        if (client_->me()) {
            ChangeMe(client_->me());
        }
    } else {
        connection_ = new Vreen::OAuthConnection(kApiKey,client_);
        connection_->setConnectionOption(Vreen::Connection::ShowAuthDialog,true);
        connection_->setScopes(kScopes);
        client_->setConnection(connection_);

        connect(connection_, SIGNAL(accessTokenChanged(QByteArray,time_t)),
                SLOT(ChangeAccessToken(QByteArray,time_t)));
        connect(client_->roster(), SIGNAL(uidChanged(int)),
                SLOT(ChangeUid(int)));
    }

    if (hasAccount_) {
        qLog(Debug) << "--- Have account";

        QSettings s;
        s.beginGroup(kSettingGroup);
        QByteArray token = s.value("token",QByteArray()).toByteArray();
        time_t expiresIn = s.value("expiresIn", 0).toUInt();
        int uid = s.value("uid",0).toInt();

        connection_->setAccessToken(token, expiresIn);
        connection_->setUid(uid);
    }

    client_->connectToHost();
}

void VkService::Logout()
{
    QSettings s;
    s.beginGroup(kSettingGroup);
    s.setValue("token", QByteArray());
    s.setValue("expiresIn",0);
    s.setValue("uid",uint(0));

    hasAccount_ = false;

    if (connection_) {
        client_->disconnectFromHost();
        connection_->clear();
        delete connection_;
        delete client_->roster();
        delete client_->me();
        connection_ = NULL;
    }

    RefreshRootSubitems();
}

void VkService::ShowConfig()
{
    app_->OpenSettingsDialogAtPage(SettingsDialog::Page_Vk);
}

void VkService::ChangeAccessToken(const QByteArray &token, time_t expiresIn)
{
    TRACE VAR(token)
    QSettings s;
    s.beginGroup(kSettingGroup);
    s.setValue("token", token);
    s.setValue("expiresIn",uint(expiresIn));
}

void VkService::ChangeUid(int uid)
{
    QSettings s;
    s.beginGroup(kSettingGroup);
    s.setValue("uid", uid);
}

void VkService::OnlineStateChanged(bool online)
{
    qLog(Debug) << "--- Online state changed to" << online;
    if (online) {
        hasAccount_ = true;
        emit LoginSuccess(true);
        RefreshRootSubitems();
        connect(client_, SIGNAL(meChanged(Vreen::Buddy*)),
                SLOT(ChangeMe(Vreen::Buddy*)));
    }
}

void VkService::ChangeMe(Vreen::Buddy *me)
{
    if (!me) {
        qLog(Warning) << "Me is NULL.";
        return;
    }

    emit NameUpdated(me->name());
    connect(me, SIGNAL(nameChanged(QString)),
            SIGNAL(NameUpdated(QString)));
    me->update(QStringList("name"));
}

void VkService::Error(Vreen::Client::Error error)
{
    QString msg;

    switch (error) {
    case Vreen::Client::ErrorApplicationDisabled:
        msg = "Application disabled";  break;
    case Vreen::Client::ErrorIncorrectSignature:
        msg = "Incorrect signature";  break;
    case Vreen::Client::ErrorAuthorizationFailed:
        msg = "Authorization failed";
        emit LoginSuccess(false);
        break;
    case Vreen::Client::ErrorToManyRequests:
        msg = "To many requests";  break;
    case Vreen::Client::ErrorPermissionDenied:
        msg = "Permission denied";  break;
    case Vreen::Client::ErrorCaptchaNeeded:
        msg = "Captcha needed";  break;
    case Vreen::Client::ErrorMissingOrInvalidParameter:
        msg = "Missing or invalid parameter";  break;
    case Vreen::Client::ErrorNetworkReply:
        msg = "Network reply";  break;
    default:
        msg = "Unknown error";
        break;
    }

    qLog(Error) << "Client error: " << error << msg;
}

void VkService::UpdateMyMusic()
{
    TRACE

    ClearStandartItem(my_music_);
    my_music_->appendRow(loading_);

    LoadSongList(0);

    connect(this, SIGNAL(SongListLoaded(int,SongList)),
            this, SLOT(MyMusicLoaded(int,SongList)));
}

void VkService::MyMusicLoaded(int id, SongList songs)
{
    TRACE VAR(id) VAR(&songs)

    if(id == 0) {
        ClearStandartItem(my_music_);
        foreach (auto song, songs) {
            my_music_->appendRow(CreateSongItem(song));
        }
    }
}


/***
 * Load song list methods
 */

void VkService::LoadSongList(int id, int count)
{
    TRACE VAR(id) VAR(count)

    auto countOfMyAudio = provider_->getCount(id);
    if (count > 0) {
        auto myAudio = provider_->getContactAudio(id,count);
        NewClosure(myAudio, SIGNAL(resultReady(QVariant)), this,
                   SLOT(SongListRecived(int,Vreen::AudioItemListReply*)),
                   id, myAudio);
    } else {
        NewClosure(countOfMyAudio, SIGNAL(resultReady(QVariant)), this,
                   SLOT(CountRecived(int, Vreen::IntReply*)),
                   id, countOfMyAudio);
    }
}

void VkService::CountRecived(int id, Vreen::IntReply* reply)
{
    TRACE VAR(id)

    int count = reply->result();
    auto myAudio = provider_->getContactAudio(id,count);
    NewClosure(myAudio, SIGNAL(resultReady(QVariant)), this,
               SLOT(SongListRecived(int,Vreen::AudioItemListReply*)),
               id, myAudio);
}

void VkService::SongListRecived(int id, Vreen::AudioItemListReply* reply)
{
    TRACE VAR(id)
    SongList songs = FromAudioList(reply->result());
    emit SongListLoaded(id, songs);
}


static inline QString ClearString(QString str) {
    /* Remove all unicode symbols */
    str = str.remove(QRegExp("^[^\\w]*"));
    str = str.remove(QRegExp("[^])\\w]*$"));
    return str;
}

SongList VkService::FromAudioList(const Vreen::AudioItemList &list)
{
    TRACE VAR(&list)

    Song song;
    SongList song_list;
    foreach (Vreen::AudioItem item, list) {
        song.set_title(ClearString(item.title()));
        song.set_artist(ClearString(item.artist()));
        song.set_length_nanosec(floor(item.duration() * kNsecPerSec));
        song.set_url(item.url());

        song_list.append(song);
    }
    ClearSimilarSongs(song_list);
    return song_list;
}


/***
 * Search
 */

int VkService::SongSearch(const QString &query, int count = 50, int offset = 0)
{
    TRACE VAR(query) VAR(count) VAR(offset)

    uint id = ++last_id_;

    auto reply = provider_->searchAudio(query,count,offset,false,Vreen::AudioProvider::SortByPopularity);
    NewClosure(reply, SIGNAL(resultReady(QVariant)), this,
               SLOT(SongSearchRecived(int,Vreen::AudioItemListReply*)),
               id, reply);

    return id;
}

void VkService::SongSearchRecived(int id, Vreen::AudioItemListReply *reply)
{
    TRACE VAR(id) VAR(reply)

    SongList songs = FromAudioList(reply->result());
    emit SongSearchResult(id, songs);
}

uint VkService::GroupSearch(const QString &query)
{
    return ++last_id_;
}

void VkService::GroupSearchRecived(int id)
{
}

/***
 * Utils
 */

void VkService::ClearStandartItem(QStandardItem * item)
{
    if (item->hasChildren()) {
        item->removeRows(0, item->rowCount());
    }
}

void VkService::ClearSimilarSongs(SongList &list)
{
    /* Search result sorted by relevance, and better quality songs usualy come first.
     * Stable sort don't mix similar song, so std::unique will remove bad quality coptes
     */

    qStableSort(list.begin(), list.end(), [](const Song &a, const Song &b){
        return (a.artist().localeAwareCompare(b.artist()) > 0)
                or (a.title().localeAwareCompare(b.title()) > 0);
    });

    int old = list.count();

    auto end = std::unique(list.begin(), list.end(), [](const Song &a, const Song &b){
        return (a.artist().localeAwareCompare(b.artist()) == 0)
                and (a.title().localeAwareCompare(b.title()) == 0);
    });

    list.erase(end, list.end());

    qDebug() << "Cleared" << old - list.count() << "items";
}
