#ifndef VKSERVICE_H
#define VKSERVICE_H

#include "internetservice.h"
#include "internetmodel.h"
#include "core/song.h"


class VkService : public InternetService
{
    Q_OBJECT
public:
    explicit VkService(Application* app, InternetModel* parent);
    ~VkService();

    static const char* kServiseName;
    static const char* kSettingGroup;
    static const char* kApiKey;

    enum ItemType {
        Type_Root = InternetModel::TypeCount,
        Type_Recommendations,
        Type_MyMusic,
        Type_Popular,
        Type_Friends,
        Type_Groups

    };

    QStandardItem* CreateRootItem();
    void LazyPopulate(QStandardItem *parent);
    
signals:
    
public slots:

private:
    QStandardItem* root_item_;
};

#endif // VKSERVICE_H
