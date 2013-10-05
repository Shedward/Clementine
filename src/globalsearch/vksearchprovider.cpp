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


#include "vksearchprovider.h"

#include <algorithm>

#include "core/logging.h"
#include "core/song.h"

VkSearchProvider::VkSearchProvider(Application* app, QObject* parent) :
  SearchProvider(app,parent),
  service_(NULL)
{
}

void VkSearchProvider::Init(VkService *service) {
  TRACE

      service_ = service;
  SearchProvider::Init("Vk.com", "vk.com",
                       QIcon(":providers/vk.png"),
                       WantsDelayedQueries | CanShowConfig);

  connect(service_, SIGNAL(SongSearchResult(RequestID,SongList)),
          this, SLOT(SongSearchResult(RequestID,SongList)));
  connect(service_, SIGNAL(GroupSearchResult(RequestID, MusicOwnerList)),
          this, SLOT(GroupSearchResult(RequestID, MusicOwnerList)));

}

void VkSearchProvider::SearchAsync(int id, const QString &query) {
  int count = service_->maxGlobalSearch();

  RequestID rid(RequestID::GlobalSearch);
  songs_recived = false;
  groups_recived = false;
  pending_searches_[rid.id()] = PendingState(id, TokenizeQuery(query));
  service_->SongSearch(rid, query,count,0);
  if (service_->isGroupsInGlobalSearch()){
    service_->GroupSearch(rid,query);
  }
}

bool VkSearchProvider::IsLoggedIn() {
  return (service_ && service_->HasAccount());
}

void VkSearchProvider::ShowConfig() {
  service_->ShowConfig();
}

void VkSearchProvider::SongSearchResult(RequestID rid, SongList songs) {
  if (rid.type() == RequestID::GlobalSearch) {
    ClearSimilarSongs(songs);
    ResultList ret;
    foreach (const Song& song, songs) {
      Result result(this);
      result.metadata_ = song;
      ret << result;
    }
    qLog(Info) << "Found" << songs.count() << "songs.";
    songs_recived = true;
    const PendingState state = pending_searches_[rid.id()];
    emit ResultsAvailable(state.orig_id_, ret);
    MaybeSearchFinished(rid.id());
  }
}

void VkSearchProvider::GroupSearchResult(RequestID rid, const MusicOwnerList &groups) {
  if (rid.type() == RequestID::GlobalSearch) {
    ResultList ret;
    foreach (const MusicOwner &group, groups) {
      Result result(this);
      result.metadata_ = group.toOwnerRadio();
      ret << result;
    }
    qLog(Info) << "Found" << groups.count() << "groups.";
    groups_recived = true;
    const PendingState state = pending_searches_[rid.id()];
    emit ResultsAvailable(state.orig_id_, ret);
    MaybeSearchFinished(rid.id());
  }
}

void VkSearchProvider::MaybeSearchFinished(int id) {
  if (pending_searches_.keys(PendingState(id, QStringList())).isEmpty() and songs_recived and groups_recived) {
    const PendingState state = pending_searches_.take(id);
    emit SearchFinished(state.orig_id_);
  }
}

void VkSearchProvider::ClearSimilarSongs(SongList &list) {
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
