/* This file is part of Clementine.
   Copyright 2013, Vlad Maltsev <shedwardx@gmail.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VKSERVICE_H
#define VKSERVICE_H

#include "internetservice.h"
#include "internetmodel.h"
#include "core/song.h"

#include "vreen/auth/oauthconnection.h"
#include "vreen/audio.h"
#include "vreen/contact.h"

#include "vkurlhandler.h"

/***
* TODO(Vk): SUMMARY
*  Cashing:
*    - Using playing stream for caching.
*      First version - return downloading filename to GStreamer.
*      But GStreamer will not wait untill the file will be downloaded, it's just skip.
*      Second version  - beforehand load next file, but it's not always possible
*      to predict correctly, for example if user start to play any other song he want.
*  Groups:
*    - Maybe store bookmarks in vk servers, for sync between platforms/computers for same user.
*    - Maybe skip group radio if user press next before next song is received or any time out
*    - Use dynamic playlist instead/with radio.
*
*  Ui:
*      - Actions should work with multiple selected items in playlist,
*          for example if user want to add many songs to his library.
*/

typedef Vreen::OAuthConnection::Scopes Scopes;

namespace Vreen {
class Client;
class OAuthConnection;
class Buddy;
}

class SearchBoxWidget;
class VkMusicCache;
class VkSearchDialog;


/***
 * Store information about user or group
 * using in bookmarks.
 */
class MusicOwner {
public:
  MusicOwner() :
    songs_count_(0),
    id_(0)
  {}

  explicit MusicOwner(const QUrl &group_url);
  Song toOwnerRadio() const;

  QString name() const { return name_; }
  int id() const { return id_; }
  int song_count() const { return songs_count_; }
  static QList<MusicOwner> parseMusicOwnerList(const QVariant &request_result);

private:
  friend QDataStream &operator <<(QDataStream &stream, const MusicOwner &val);
  friend QDataStream &operator >>(QDataStream &stream, MusicOwner &val);
  friend QDebug operator<< (QDebug d, const MusicOwner &owner);

  int songs_count_;
  int id_; // if id > 0 is user otherwise id group
  QString name_;
  //name used in url http://vk.com/<screen_name> for example: http://vk.com/shedward
  QString screen_name_;
  QUrl photo_;
};

typedef QList<MusicOwner> MusicOwnerList;

Q_DECLARE_METATYPE(MusicOwner)

QDataStream& operator<<(QDataStream & stream, const MusicOwner & val);
QDataStream& operator>>(QDataStream & stream, MusicOwner & var);
QDebug operator<< (QDebug d, const MusicOwner &owner);



/***
 * The simple structure allows the handler to determine
 * how to react to the received request or quickly skip unwanted.
 */
struct SearchID {

  enum Type {
    GlobalSearch,
    LocalSearch,
    MoreLocalSearch,
    UserOrGroup
  };

  SearchID(Type type)
    : type_(type) {
    id_= last_id_++;
  }
  int id() const { return id_; }
  Type type() const { return type_; }
private:
  static uint last_id_;
  int id_;
  Type type_;
};



/***
 * VkService
 */
class VkService : public InternetService {
  Q_OBJECT
public:
  explicit VkService(Application* app, InternetModel* parent);
  ~VkService();

  static const char* kServiceName;
  static const char* kSettingGroup;
  static const char* kUrlScheme;
  static const uint  kApiKey;
  static const Scopes kScopes;
  static const char* kDefCacheFilename;
  static QString kDefCacheDir();
  static const int kMaxVkSongList;
  static const int kCustomSongCount;

  enum ItemType {
    Type_Root = InternetModel::TypeCount,

    Type_NeedLogin,

    Type_Loading,
    Type_More,

    Type_Recommendations,
    Type_MyMusic,
    Type_Bookmark,
    Type_Album,

    Type_Search
  };

  enum Role { Role_MusicOwnerMetadata = InternetModel::RoleCount,
              Role_AlbumMetadata };

  /* InternetService interface */
  QStandardItem* CreateRootItem();
  void LazyPopulate(QStandardItem *parent);
  void ShowContextMenu(const QPoint &global_pos);
  void ItemDoubleClicked(QStandardItem *item);
  QList<QAction*> playlistitem_actions(const Song &song);
  Application* app() { return app_; }

  /* Interface*/
  void RefreshRootSubitems();
  QWidget* HeaderWidget() const;

  /* Connection */
  void Login();
  void Logout();
  bool HasAccount() const { return hasAccount_; }
  bool WaitForReply(Vreen::Reply *reply);

  /* Music */
  VkMusicCache* cache() { return cache_; }
  void SetCurrentSongFromUrl(const QUrl &url); // Used if song taked from cache.
  QUrl GetSongPlayUrl(const QUrl &url, bool is_playing = true);
  // Return random song result from group playlist.
  UrlHandler::LoadResult GetGroupNextSongUrl(const QUrl& url);

  void SongSearch(SearchID id,const QString &query, int count = 50, int offset = 0);
  void GroupSearch(SearchID id, const QString &query);

  /* Settings */
  void UpdateSettings();
  int maxGlobalSearch() { return maxGlobalSearch_; }
  bool isCachingEnabled() { return cachingEnabled_; }
  bool isGroupsInGlobalSearch() { return groups_in_global_search_; }
  QString cacheDir() { return cacheDir_; }
  QString cacheFilename() { return cacheFilename_; }
  bool isLoveAddToMyMusic() { return love_is_add_to_mymusic_; }

signals:
  void NameUpdated(QString name);
  void ConnectionStateChanged(Vreen::Client::State state);
  void LoginSuccess(bool);
  void SongSearchResult(SearchID id, const SongList &songs);
  void GroupSearchResult(SearchID id, const MusicOwnerList &groups);
  void UserOrGroupSearchResult(SearchID id, const MusicOwnerList &owners);
  void StopWaiting();

public slots:
  void ShowConfig();
  void FindUserOrGroup(const QString &q);

private slots:
  /* Connection */
  void ChangeAccessToken(const QByteArray &token, time_t expiresIn);
  void ChangeUid(int uid);
  void ChangeConnectionState(Vreen::Client::State state);
  void ChangeMe(Vreen::Buddy*me);
  void Error(Vreen::Client::Error error);

  /* Music */
  void UpdateMyMusic();
  void UpdateBookmarkSongs();
  void LoadBookmarkSongs(QStandardItem *item);
  void UpdateAlbumSongs();
  void LoadAlbumSongs(QStandardItem *item);
  void FindSongs(QString query);
  void FindMore();
  void UpdateRecommendations();
  void MoreRecommendations();
  void FindThisArtist();
  void AddToMyMusic();
  void AddToMyMusicCurrent();
  void RemoveFromMyMusic();
  void AddToCache();
  void CopyShareUrl();
  void ShowSearchDialog();

  void AddSelectedToBookmarks();
  void RemoveFromBookmark();

  void SongSearchRecived(SearchID id, Vreen::AudioItemListReply *reply);
  void GroupSearchRecived(SearchID id, Vreen::Reply *reply);
  void UserOrGroupRecived(SearchID id, Vreen::Reply *reply);
  void AlbumListRecived(Vreen::AudioAlbumItemListReply *reply);

  void AppendLoadedSongs(QStandardItem* item, Vreen::AudioItemListReply *reply);
  void RecommendationsLoaded(Vreen::AudioItemListReply *reply);
  void SearchResultLoaded(SearchID rid, const SongList &songs);

private:
  /* Interface */
  QStandardItem *CreateAndAppendRow(QStandardItem *parent, VkService::ItemType type);
  void ClearStandartItem(QStandardItem*item);
  QStandardItem * GetBookmarkItemById(int id);
  void CreateMenu();
  QStandardItem* root_item_;
  QStandardItem* recommendations_;
  QStandardItem* my_music_;
  QStandardItem* search_;

  QMenu* context_menu_;

  QAction* update_my_music_;
  QAction* update_recommendations_;
  QAction* update_bookmark_;
  QAction* update_album_;
  QAction* find_this_artist_;
  QAction* add_to_my_music_;
  QAction* remove_from_my_music_;
  QAction* add_song_to_cache_;
  QAction* copy_share_url_;
  QAction* add_to_bookmarks_;
  QAction* remove_from_bookmarks_;
  QAction* find_owner_;

  SearchBoxWidget* search_box_;
  VkSearchDialog* vk_search_dialog_;

  /* Connection */
  Vreen::Client *client_;
  Vreen::OAuthConnection  *connection_;
  bool hasAccount_;
  int my_id_;
  QByteArray token_;
  time_t expiresIn_;
  VkUrlHandler* url_handler_;

  /* Music */
  void LoadAndAppendSongList(QStandardItem *item, int uid, int album_id = -1);
  Song FromAudioItem(const Vreen::AudioItem &item);
  SongList FromAudioList(const Vreen::AudioItemList &list);
  void AppendSongs(QStandardItem *parent, const SongList &songs);

  QStandardItem *AppendBookmark(const MusicOwner &owner);
  void SaveBookmarks();
  void LoadBookmarks();

  void LoadAlbums();
  QStandardItem *AppendAlbum(const Vreen::AudioAlbumItem &album);

  Vreen::AudioProvider* audio_provider_;
  VkMusicCache* cache_;
  // Keeping when more recent results recived.
  // Using for prevent loading tardy result instead.
  uint last_search_id_;
  QString last_query_;
  Song selected_song_; // Store for context menu actions.
  Song current_song_; // Store for actions with now playing song.
  // Store current group url for actions with it.
  QUrl current_group_url_;

  /* Settings */
  int maxGlobalSearch_;
  bool cachingEnabled_;
  bool love_is_add_to_mymusic_;
  bool groups_in_global_search_;
  QString cacheDir_;
  QString cacheFilename_;
};

#endif // VKSERVICE_H
