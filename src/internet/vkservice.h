#ifndef VKSERVICE_H
#define VKSERVICE_H

#include "internetservice.h"
#include "internetmodel.h"
#include "core/song.h"

#include <boost/scoped_ptr.hpp>

#include "vreen/auth/oauthconnection.h"
#include "vreen/audio.h"
#include "vreen/contact.h"

#include "vkurlhandler.h"

#define  VAR(var) qLog(Debug) << ("---    where " #var " =") << (var);
#define  TRACE qLog(Debug) << "--- " << __PRETTY_FUNCTION__ ;

typedef Vreen::OAuthConnection::Scopes Scopes;
typedef uint GroupID;

namespace Vreen {
class Client;
class OAuthConnection;
class Buddy;
}

class SearchBoxWidget;

class VkService : public InternetService
{
    Q_OBJECT
public:
    explicit VkService(Application* app, InternetModel* parent);
    ~VkService();

    static const char* kServiceName;
    static const char* kSettingGroup;
    static const char* kUrlScheme;
    static const uint  kApiKey;
    static const Scopes kScopes;

    enum ItemType {        
        Type_Root = InternetModel::TypeCount,

        Type_NeedLogin,

        Type_Loading,
        Type_More,

        Type_Recommendations,
        Type_MyMusic,

        Type_Group,
        Type_Friend,
        Type_Playlist,
        Type_Search
    };

    enum RequestType {
        GlobalSearch,
        LocalSearch,
        MoreLocalSearch,
        UserAudio,
        UserRecomendations
    };

    struct RequestID {
        RequestID(RequestType type, int id = 0)
            : type_(type)
        {
            switch (type) {
            case UserAudio:
            case UserRecomendations:
                id_ = id;
                break;
            default:
                id_= last_id_++;
                break;
            }
        }

        int id() const { return id_; }
        RequestType type() const { return type_; }

    private:
        static uint last_id_;
        int id_;
        RequestType type_;
    };

    /* InternetService interface */
    QStandardItem* CreateRootItem();
    void LazyPopulate(QStandardItem *parent);
    void ShowContextMenu(const QPoint &global_pos);
    void ItemDoubleClicked(QStandardItem *item);

    /* Interface*/
    void RefreshRootSubitems();
    QWidget* HeaderWidget() const;

    /* Connection */
    void Login();
    void Logout();
    bool hasAccount() const { return hasAccount_; }
    bool WaitForReply(Vreen::Reply *reply);

    /* Music */
    QUrl GetSongUrl(QString song_id);

    void SongSearch(RequestID id,const QString &query, int count = 50, int offset = 0);
    int GroupSearch(const QString &query, int count, int offset);

    void UpdateMyMusic();
    void UpdateRecommendations();
    void MoreRecommendations();
    Q_SLOT void Search(QString query);
    void MoreSearch();

signals:
    void NameUpdated(QString name);
    void LoginSuccess(bool succ);

    void SongListLoaded(RequestID id, SongList songs);

    void SongSearchResult(RequestID id, const SongList &songs);
    void GroupSearchResult(int id, Vreen::GroupList groups);
    
public slots:
    void ShowConfig();
    void LoadSongList(uint uid, int count = 0); // zero means - load full list

private slots:
    /* Connection */
    void ChangeAccessToken(const QByteArray &token, time_t expiresIn);
    void ChangeUid(int uid);
    void OnlineStateChanged(bool online);
    void ChangeMe(Vreen::Buddy*me);
    void Error(Vreen::Client::Error error);

    /* Music */
    void SongListRecived(RequestID rid, Vreen::AudioItemListReply *reply);
    void CountRecived(RequestID rid, Vreen::IntReply* reply);
    void SongSearchRecived(RequestID id, Vreen::AudioItemListReply *reply);
    void GroupSearchRecived(int id);

    void MyMusicLoaded(RequestID rid, const SongList &songs);
    void RecommendationsLoaded(RequestID id, const SongList &songs);
    void SearchLoaded(RequestID id, const SongList &songs);

private:
    /* Interface */
    QStandardItem *CreateAndAppendRow(QStandardItem *parent, VkService::ItemType type);
    void ClearStandartItem(QStandardItem*item);
    QStandardItem* root_item_;
    QStandardItem* recommendations_;
    QStandardItem* my_music_;
    QStandardItem* search_;
    QVector<QStandardItem*> playlists_;
    boost::scoped_ptr<QMenu> context_menu_;
    SearchBoxWidget* search_box_;

    /* Connection */
    Vreen::Client *client_;
    Vreen::OAuthConnection *connection_;
    bool hasAccount_;
    VkUrlHandler* url_handler_;

    /* Music */
    Vreen::AudioProvider* provider_;
    uint last_search_id_;
    QString last_query_;
    SongList FromAudioList(const Vreen::AudioItemList &list);
    void AppendSongs(QStandardItem *parent, const SongList &songs);
};

#endif // VKSERVICE_H
