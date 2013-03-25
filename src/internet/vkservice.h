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
        Type_MoreRecommendations,

        Type_Recommendations,
        Type_MyMusic,

        Type_Group,
        Type_Friend,
        Type_Playlist,
        Type_Search
    };    QStandardItem* loading_;
    QStandardItem* more_recommendations;

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
    int SongSearch(const QString &query, int count, int offset);
    int GroupSearch(const QString &query, int count, int offset);
    void UpdateMyMusic();
    void UpdateRecommendations();
    void MoreRecommendations();

signals:
    void NameUpdated(QString name);
    void LoginSuccess(bool succ);

    void SongListLoaded(int id, SongList songs);

    void SongSearchResult(int id, SongList songs);
    void GroupSearchResult(int id, Vreen::GroupList groups);
    
public slots:
    void ShowConfig();
    void LoadSongList(int id, int count = 0); // zero means - load full list
    void Search(QString query);

private slots:
    /* Connection */
    void ChangeAccessToken(const QByteArray &token, time_t expiresIn);
    void ChangeUid(int uid);
    void OnlineStateChanged(bool online);
    void ChangeMe(Vreen::Buddy*me);
    void Error(Vreen::Client::Error error);

    /* Music */
    void SongListRecived(int id, Vreen::AudioItemListReply *reply);
    void CountRecived(int id, Vreen::IntReply* reply);
    void SongSearchRecived(int id, Vreen::AudioItemListReply *reply);
    void GroupSearchRecived(int id);

    void MyMusicLoaded(int id, SongList songs);
    void RecommendationsLoaded(int id, SongList songs);

private:
    /* Interface */
    QStandardItem *CreateAndAppendRow(QStandardItem *parent, VkService::ItemType type);
    void ClearStandartItem(QStandardItem*item);
    QStandardItem* root_item_;
    QStandardItem* recommendations_;
    QStandardItem* my_music_;
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
    uint last_id_;
    SongList FromAudioList(const Vreen::AudioItemList &list);
    void ClearSimilarSongs(SongList &list);
};

#endif // VKSERVICE_H
