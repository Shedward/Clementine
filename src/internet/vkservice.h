#ifndef VKSERVICE_H
#define VKSERVICE_H

#include "internetservice.h"
#include "internetmodel.h"
#include "core/song.h"

#include <boost/scoped_ptr.hpp>

#include "vreen/auth/oauthconnection.h"
#include "vreen/audio.h"

typedef Vreen::OAuthConnection::Scopes Scopes;
typedef uint GroupID;

namespace Vreen {
class Client;
class OAuthConnection;
class Buddy;
}

class VkService : public InternetService
{
    Q_OBJECT
public:
    explicit VkService(Application* app, InternetModel* parent);
    ~VkService();

    static const char* kServiceName;
    static const char* kSettingGroup;
    static const uint  kApiKey;
    static const Scopes kScopes;

    enum ItemType {        
        Type_Root = InternetModel::TypeCount,

        Type_NeedLogin,

        Type_Recommendations,
        Type_MyMusic,

        Type_Group,
        Type_Friend,
        Type_Playlist,
        Type_Search
    };

    /* InternetService interface */
    QStandardItem* CreateRootItem();
    void LazyPopulate(QStandardItem *parent);
    void ShowContextMenu(const QPoint &global_pos);
    void ItemDoubleClicked(QStandardItem *item);

    /* Interface*/
    void RefreshRootSubitems();

    /* Connection */
    void Login();
    void Logout();
    bool hasAccount() const { return hasAccount_; }

    /* Music */
    uint SongSearch(const QString &query);
    uint GroupSearch(const QString &query);

signals:
    void NameUpdated(QString name);
    void LoginSuccess(bool succ);

    void SongSearchResult(uint id, const SongList &songs);
    void GroupSearchResult(uint id, const QVector<GroupID> &groups);
    
public slots:
    void ShowConfig();

private slots:
    /* Connection */
    void ChangeAccessToken(const QByteArray &token, time_t expiresIn);
    void ChangeUid(int uid);
    void OnlineStateChanged(bool online);
    void ChangeMe(Vreen::Buddy*me);
    void Error(Vreen::Client::Error error);

    /* Music */

    void MyMusicRecived();
    void MyAudioCountRecived();

    void SongSearchFinished(int id);
    void GroupSearchFinished(int id);

private:
    /* Interface */
    QStandardItem* CreateStandartItem();

    QStandardItem* need_login_;
    QStandardItem* root_item_;
    QStandardItem* recommendations_;
    QStandardItem* my_music_;
    QVector<QStandardItem*> playlists_;
    boost::scoped_ptr<QMenu> context_menu_;

    /* Connection */
    Vreen::Client *client_;
    Vreen::OAuthConnection *connection_;
    bool hasAccount_;

    /* Music */
    Vreen::AudioProvider* provider_;
    SongList FromAudioList(const Vreen::AudioItemList &list);
    void LoadMyMusic();
};

#endif // VKSERVICE_H
