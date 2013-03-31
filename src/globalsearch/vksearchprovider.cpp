#include "vksearchprovider.h"

#include <algorithm>

#include "core/logging.h"

VkSearchProvider::VkSearchProvider(Application* app, QObject* parent) :
    SearchProvider(app,parent),
    service_(NULL)
{
}

void VkSearchProvider::Init(VkService *service)
{
    TRACE

    service_ = service;
    SearchProvider::Init("Vk.com", "vk.com",
                         QIcon(":providers/vk.png"),
                         WantsDelayedQueries | CanShowConfig);

    connect(service_, SIGNAL(SongSearchResult(RequestID,SongList)),
            this, SLOT(SongSearchResult(RequestID,SongList)));

}

void VkSearchProvider::SearchAsync(int id, const QString &query)
{
    TRACE VAR(id) VAR(query);

    QSettings s;
    s.beginGroup(VkService::kSettingGroup);

    int count = s.value("maxSearchResult",100).toInt();

    RequestID rid(VkService::GlobalSearch);
    service_->SongSearch(rid, query,count,0);
    pending_searches_[rid.id()] = PendingState(id, TokenizeQuery(query));
}

bool VkSearchProvider::IsLoggedIn()
{
    return (service_ && service_->HasAccount());
}

void VkSearchProvider::ShowConfig()
{
    service_->ShowConfig();
}

void VkSearchProvider::SongSearchResult(VkService::RequestID rid, SongList songs)
{
    TRACE VAR(rid.id()) VAR(&songs);

    // Map back to the original id.
    if (rid.type() == VkService::GlobalSearch) {
        const PendingState state = pending_searches_.take(rid.id());
        const int global_search_id = state.orig_id_;

        ClearSimilarSongs(songs);

        ResultList ret;

        foreach (const Song& song, songs) {
            Result result(this);
            result.metadata_ = song;
            ret << result;
        }

        emit ResultsAvailable(global_search_id, ret);
        MaybeSearchFinished(global_search_id);
   }
}

void VkSearchProvider::GroupSearchResult(int id, const QVector<GroupID> &groups)
{
}

void VkSearchProvider::GroupSongLoaded(GroupID id, SongList &songs)
{
    //NOTE: It;s realy needed
}

void VkSearchProvider::MaybeSearchFinished(int id)
{
    if (pending_searches_.keys(PendingState(id, QStringList())).isEmpty()) {
      emit SearchFinished(id);
    }
}

void VkSearchProvider::ClearSimilarSongs(SongList &list)
{
    /* Search result sorted by relevance, and better quality songs usualy come first.
     * Stable sort don't mix similar song, so std::unique will remove bad quality copies.
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
