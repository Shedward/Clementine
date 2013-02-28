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

    static const char* kServiceName;
    static const char* kSettingGroup;
    static const char* kApiKey;

    enum ItemType {
        Type_Root = InternetModel::TypeCount,
        Type_Recommendations,
        Type_MyMusic,

        Type_Group,
        Type_Friend,
        Type_Playlist,
        Type_Search
    };

    QStandardItem* CreateRootItem();
    void LazyPopulate(QStandardItem *parent);
    
signals:
    
public slots:
    void addSearchPlaylist(const QString &query);
    void addUserPlaylist(uint uid, uint aid);
    void addGroupPlaylist(uint gid, uint aid);

    void ShowConfig();

private:
    QStandardItem* CreateStandartItem();

    QStandardItem* root_item_;
    QStandardItem* recommendations_;
    QStandardItem* my_music_;
    QVector<QStandardItem*> playlists_;
};

#endif // VKSERVICE_H
