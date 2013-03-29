#include <math.h>

#include <QMenu>
#include <QSettings>
#include <QByteArray>
#include <QEventLoop>
#include <QTimer>

#include <boost/scoped_ptr.hpp>

#include "core/application.h"
#include "core/closure.h"
#include "core/logging.h"
#include "core/mergedproxymodel.h"
#include "core/player.h"
#include "core/timeconstants.h"
#include "ui/iconloader.h"

#include "internetmodel.h"
#include "internetplaylistitem.h"
#include "globalsearch/globalsearch.h"
#include "searchboxwidget.h"

#include "vreen/auth/oauthconnection.h"
#include "vreen/audio.h"
#include "vreen/contact.h"
#include "vreen/roster.h"

#include "globalsearch/vksearchprovider.h"
#include "vkservice.h"

const char*  VkService::kServiceName = "Vk.com";
const char*  VkService::kSettingGroup = "Vk.com";
const char*  VkService::kUrlScheme = "vk";
const uint   VkService::kApiKey = 3421812;
const Scopes VkService::kScopes =
        Vreen::OAuthConnection::Offline |
        Vreen::OAuthConnection::Audio |
        Vreen::OAuthConnection::Friends |
        Vreen::OAuthConnection::Groups;


VkService::VkService(Application *app, InternetModel *parent) :
    InternetService(kServiceName, app, parent, parent),
    root_item_(nullptr),
    recommendations_(nullptr),
    my_music_(nullptr),
    search_(nullptr),
    context_menu_(new QMenu),
    search_box_(new SearchBoxWidget(this)),
    client_(new Vreen::Client),
    connection_(nullptr),
    hasAccount_(false),
    url_handler_(new VkUrlHandler(this, this)),
    provider_(nullptr),
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
    hasAccount_ = (uid != 0) and !token.isEmpty();

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
 app_->player()->RegisterUrlHandler(url_handler_);
    connect(search_box_, SIGNAL(TextChanged(QString)), SLOT(Search(QString)));
}

VkService::~VkService()
{
}

/***
 * Interface
 */

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
    case Type_MyMusic:
        UpdateMyMusic();
        break;
    case Type_Recommendations:
        UpdateRecommendations();
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
    switch (item->data(InternetModel::Role_Type).toInt()) {
    case Type_NeedLogin:
        ShowConfig();
        break;
    case Type_More:
        MoreRecommendations();
        break;
    }
}

void VkService::ShowConfig()
{
    app_->OpenSettingsDialogAtPage(SettingsDialog::Page_Vk);
}

void VkService::RefreshRootSubitems()
{
    ClearStandartItem(root_item_);

    if (hasAccount_) {
        CreateAndAppendRow(root_item_, Type_Recommendations);
        CreateAndAppendRow(root_item_, Type_MyMusic);
    } else {
        CreateAndAppendRow(root_item_, Type_NeedLogin);
    }
}

QWidget *VkService::HeaderWidget() const
{
    if (hasAccount()) {
        return search_box_;
    } else {
        return nullptr;
    }
}

/***
 * Connection
 */

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
        connection_ = nullptr;
    }

    RefreshRootSubitems();
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
        qLog(Warning) << "Me is nullptr.";
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

/***
 * My Music
 */

void VkService::UpdateMyMusic()
{
    TRACE

    ClearStandartItem(my_music_);
    CreateAndAppendRow(my_music_,Type_Loading);

    LoadSongList(0);

    connect(this, SIGNAL(SongListLoaded(int,SongList)),
            this, SLOT(MyMusicLoaded(int,SongList)));
}

void VkService::MyMusicLoaded(int id, SongList songs)
{
    TRACE VAR(id) VAR(&songs)

    if(id == 0) {
        ClearStandartItem(my_music_);
        AppendSongs(my_music_,songs);
    }
}

/***
 * Recommendation
 */

void VkService::UpdateRecommendations()
{
    TRACE

    ClearStandartItem(recommendations_);
    CreateAndAppendRow(recommendations_,Type_Loading);

    auto myAudio = provider_->getRecommendationsForUser(0,50,0);
    NewClosure(myAudio, SIGNAL(resultReady(QVariant)), this,
               SLOT(SongListRecived(int,Vreen::AudioItemListReply*)),
               -1, myAudio);

    connect(this, SIGNAL(SongListLoaded(int,SongList)),
            this, SLOT(RecommendationsLoaded(int,SongList)));
}

inline static void RemoveLastRow(QStandardItem* item){
    item->removeRow(item->rowCount() - 1);
}

void VkService::MoreRecommendations()
{
    TRACE

    RemoveLastRow(recommendations_); // Last row is "More"
    CreateAndAppendRow(recommendations_,Type_Loading);
    auto myAudio = provider_->getRecommendationsForUser(0,50,recommendations_->rowCount()-1);

    NewClosure(myAudio, SIGNAL(resultReady(QVariant)), this,
               SLOT(SongListRecived(int,Vreen::AudioItemListReply*)),
               -1, myAudio);
}

void VkService::RecommendationsLoaded(int id, SongList songs)
{
    TRACE VAR(id) VAR(&songs)

    if(id == -1) {
        RemoveLastRow(recommendations_); // Last row is "Loading..."
        AppendSongs(recommendations_,songs);
        CreateAndAppendRow(recommendations_,Type_More);
    }
}

/***
 * Search
 */

void VkService::Search(QString query)
{
    if (query.isEmpty()) {
        root_item_->removeRow(search_->row());
        search_ = nullptr;
        search_id_ = 0;
    } else {
        if (!search_) {
            CreateAndAppendRow(root_item_,Type_Search);
            connect(this, SIGNAL(SongSearchResult(int,SongList)),
                    SLOT(SearchLoaded(int,SongList)));
        }
        search_id_ = SongSearch(query);
    }
}

void VkService::SearchLoaded(int id, SongList songs)
{
    if (id == search_id_){
        if (search_) {
            ClearStandartItem(search_);
            if (songs.count() > 0) {
                AppendSongs(search_, songs);
            } else {
                search_->appendRow(new QStandardItem("Nothing found"));
            }
            QModelIndex index = model()->merged_model()->mapFromSource(search_->index());
            ScrollToIndex(index);
        }
    }
}

/***
 * Load song list methods
 */

QUrl VkService::GetSongUrl(QString song_id)
{
    auto audioList = provider_->getAudioByIds(song_id);
    WaitForReply(audioList);
    Vreen::AudioItem song = audioList->result()[0];
    return song.url();
}

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


static QString ClearString(QString str) {
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

        QString url = QString("vk://song/%1_%2").
                arg(item.ownerId()).
                arg(item.id());

        qDebug() << url;
        song.set_url(QUrl(url));

        song_list.append(song);
    }

    return song_list;
}


/***
 * Search
 */

int VkService::SongSearch(const QString &query, int count, int offset)
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

int VkService::GroupSearch(const QString &query, int count, int offset)
{
    return ++last_id_;
}

void VkService::GroupSearchRecived(int id)
{
}

/***
 * Utils
 */
QStandardItem* VkService::CreateAndAppendRow(QStandardItem *parent, VkService::ItemType type){

    QStandardItem* item;

    switch (type) {
    case Type_NeedLogin:
        item = new QStandardItem(
                    QIcon(),
                    tr("Double click to login")
                    );
        item->setData(InternetModel::PlayBehaviour_DoubleClickAction,
                             InternetModel::Role_PlayBehaviour);
    case Type_Loading:
        item = new QStandardItem(
                    QIcon(),
                    tr("Loading...")
                    );
        break;

    case Type_More:
        item = new QStandardItem(
                    QIcon(),
                    tr("More")
                    );
        item->setData(InternetModel::PlayBehaviour_DoubleClickAction,
                             InternetModel::Role_PlayBehaviour);
        break;

    case Type_Recommendations:
        item = new QStandardItem(
                    QIcon(":vk/recommends.png"),
                    tr("My Recommendations"));
        item->setData(true, InternetModel::Role_CanLazyLoad);
        item->setData(InternetModel::PlayBehaviour_MultipleItems,
                                  InternetModel::Role_PlayBehaviour);
        recommendations_ = item;
        break;

    case Type_MyMusic:
        item = new QStandardItem(
                    QIcon(":vk/my_music.png"),
                    tr("My Music"));
        item->setData(true, InternetModel::Role_CanLazyLoad);
        item->setData(InternetModel::PlayBehaviour_MultipleItems,
                           InternetModel::Role_PlayBehaviour);
        my_music_ = item;
        break;

    case Type_Search:
        item = new QStandardItem(
                    QIcon(":vk/find.png"),
                    tr("Search"));
        item->setData(InternetModel::PlayBehaviour_MultipleItems,
                           InternetModel::Role_PlayBehaviour);
        search_ = item;
    default:
        break;
    }

    item->setData(type, InternetModel::Role_Type);
    parent->appendRow(item);
    return item;
}

void VkService::AppendSongs(QStandardItem *parent, const SongList &songs)
{
    foreach (auto song, songs) {
        parent->appendRow(CreateSongItem(song));
    }
}

void VkService::ClearStandartItem(QStandardItem * item)
{
    if (item->hasChildren()) {
        item->removeRows(0, item->rowCount());
    }
}

bool VkService::WaitForReply(Vreen::Reply* reply) {
    QEventLoop event_loop;
    QTimer timeout_timer;
    connect(&timeout_timer, SIGNAL(timeout()), &event_loop, SLOT(quit()));
    connect(reply, SIGNAL(resultReady(QVariant)), &event_loop, SLOT(quit()));
    timeout_timer.start(10000);
    event_loop.exec();
    if (!timeout_timer.isActive()) {
      qLog(Error) << "Vk.com request timeout";
      return false;
    }
    timeout_timer.stop();
    return true;
}
