#ifndef VKSEARCHPROVIDER_H
#define VKSEARCHPROVIDER_H

#include "internet/vkservice.h"

#include "searchprovider.h"

typedef uint GroupID;

class VkService;

class VkSearchProvider : public SearchProvider
{
    Q_OBJECT
public:
    VkSearchProvider(Application* app, QObject* parent = 0);
    void Init(VkService* service);
    void SearchAsync(int id, const QString &query);
    QStringList GetSuggestions(int count);
    bool IsLoggedIn();
    void ShowConfig();
    InternetService* internet_service() { return service_; }
    
public slots:
    void SongSearchResult(int id, const SongList& songs);
    void GroupSearchResult(int id, const QVector<GroupID>& groups);
    void GroupSongLoaded(GroupID id, SongList& songs);

    void MaybeSearchFinished(int id);
private:
    VkService* service_;
    QImage icon_;
    QMap<int, PendingState> pending_searches_;
};

#endif // VKSEARCHPROVIDER_H
