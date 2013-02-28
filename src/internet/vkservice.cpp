#include "vkservice.h"

#include "core/application.h"

const char* VkService::kServiceName = "Vk.com";
const char* VkService::kSettingGroup = "Vk.com";

VkService::VkService(Application *app, InternetModel *parent) :
    InternetService(kServiceName, app, parent, parent)
{
}

VkService::~VkService()
{
}


QStandardItem *VkService::CreateRootItem()
{
    root_item_ = new QStandardItem(QIcon(":providers/vk.png"),kServiceName);
    root_item_->setData(true, InternetModel::Role_CanLazyLoad);
    return root_item_;
}

void VkService::LazyPopulate(QStandardItem *parent)
{
    switch (parent->data(InternetModel::Role_Type).toInt()) {
    case InternetModel::Type_Service:

        recommendations_ = new QStandardItem(
                    QIcon(":vk/recommends.png"),
                    tr("My Recommendations"));
        recommendations_->setData(Type_Recommendations, InternetModel::Role_Type);
        parent->appendRow(recommendations_);

        my_music_ = new QStandardItem(
                    QIcon(":vk/my_music.png"),
                    tr("My Music"));
        my_music_->setData(Type_MyMusic, InternetModel::Role_CanLazyLoad);
        parent->appendRow(my_music_);

        break;
    default:
        break;
    }
}

void VkService::addSearchPlaylist(const QString &query)
{
}

void VkService::addUserPlaylist(uint uid, uint aid)
{
}

void VkService::addGroupPlaylist(uint gid, uint aid)
{
}

void VkService::ShowConfig()
{
    app_->OpenSettingsDialogAtPage(SettingsDialog::Page_Vk);
}
