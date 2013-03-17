#include "vksearchprovider.h"

VkSearchProvider::VkSearchProvider(Application* app, QObject* parent) :
    SearchProvider(app,parent),
    service_(NULL)
{
}

void VkSearchProvider::Init(VkService *service)
{
    service_ = service;
    SearchProvider::Init("Vk.com", "vk.com",
                         QIcon(":providers/vk.png"),
                         WantsDelayedQueries | CanGiveSuggestions | CanShowConfig);

}

void VkSearchProvider::SearchAsync(int id, const QString &query)
{
}

QStringList VkSearchProvider::GetSuggestions(int count)
{
    return QStringList("Temporary list");
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
