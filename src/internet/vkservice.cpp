#include "vkservice.h"

const char* VkService::kServiseName = "Vk.com";

VkService::VkService(Application *app, InternetModel *parent) :
    InternetService(kServiseName, app, parent, parent)
{
}

VkService::~VkService()
{
}


QStandardItem *VkService::CreateRootItem()
{
    root_item_ = new QStandardItem(QIcon(":providers/vk.ico"),kServiseName);
   // root_item_->setData(true, );
    return root_item_;
}

void VkService::LazyPopulate(QStandardItem *parent)
{
}
