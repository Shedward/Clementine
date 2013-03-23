#include "vksearchprovider.h"
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

    connect(service_, SIGNAL(SongSearchResult(int,SongList)),
            this, SLOT(SongSearchResult(int,SongList)));

}

void VkSearchProvider::SearchAsync(int id, const QString &query)
{
    TRACE VAR(id) VAR(query);

    QSettings s;
    s.beginGroup(VkService::kSettingGroup);

    int count = s.value("maxSearchResult",100).toInt();

    const int service_id = service_->SongSearch(query,count,0);
    pending_searches_[service_id] = PendingState(id, TokenizeQuery(query));
}

bool VkSearchProvider::IsLoggedIn()
{
    return (service_ && service_->hasAccount());
}

void VkSearchProvider::ShowConfig()
{
    service_->ShowConfig();
}

void VkSearchProvider::SongSearchResult(int id, const SongList &songs)
{
    TRACE VAR(id) VAR(&songs)

    // Map back to the original id.
    const PendingState state = pending_searches_.take(id);
    const int global_search_id = state.orig_id_;

    ResultList ret;

    foreach (const Song& song, songs) {
      Result result(this);
      result.metadata_ = song;
      ret << result;
    }

    emit ResultsAvailable(global_search_id, ret);
    MaybeSearchFinished(global_search_id);
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
