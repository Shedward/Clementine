#include "vkurlhandler.h"

#include "core/logging.h"

#include "vkservice.h"

VkUrlHandler::VkUrlHandler(VkService *service, QObject *parent)
    : UrlHandler(parent),
      service_(service)
{
}

UrlHandler::LoadResult VkUrlHandler::StartLoading(const QUrl &url)
{
    TRACE VAR(url);

    QStringList args = url.toString().remove("vk://").split("/");
    if (args.size() < 2) {
        qLog(Error) << "Invalid VK.com URL: " << url.toString();
    } else {
        QString action = args[0];
        QString id = args[1];

        if (action == "song") {
            QUrl media_url = service_->GetSongUrl(id);
            return LoadResult(url,LoadResult::TrackAvailable,media_url);

        } else if (action == "group") {

        } else {
            qLog(Error) << "Invalid vk.com url action:" << action;
        }
    }
    return LoadResult();
}
