#ifndef VKSEARCHPROVIDER_H
#define VKSEARCHPROVIDER_H

#include "internet/vkservice.h"

#include "searchprovider.h"

typedef uint GroupID;
typedef VkService::RequestID RequestID;

class VkSearchProvider : public SearchProvider
{
    Q_OBJECT
public:
    VkSearchProvider(Application* app, QObject* parent = 0);
    void Init(VkService* service);
    void SearchAsync(int id, const QString &query);
    bool IsLoggedIn();
    void ShowConfig();
    InternetService* internet_service() { return service_; }
    
public slots:
    void SongSearchResult(RequestID rid, SongList songs);
    void GroupSearchResult(int id, const QVector<GroupID>& groups);
    void GroupSongLoaded(GroupID id, SongList& songs);

private:
    void MaybeSearchFinished(int id);
    void ClearSimilarSongs(SongList &list);
    VkService* service_;
    QMap<int, PendingState> pending_searches_;
};

#endif // VKSEARCHPROVIDER_H


