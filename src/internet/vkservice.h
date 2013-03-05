#ifndef VKSERVICE_H
#define VKSERVICE_H

#include "vreen/client.h"

#include "internetservice.h"
#include "internetmodel.h"
#include "core/song.h"

#include <boost/scoped_ptr.hpp>


class VkService : public InternetService
{
    Q_OBJECT
public:
    explicit VkService(Application* app, InternetModel* parent);
    ~VkService();

    static const char* kServiceName;
    static const char* kSettingGroup;
    static const uint  kApiKey;

    enum ItemType {        
        Type_Root = InternetModel::TypeCount,

        Type_NeedLogin,

        Type_Recommendations,
        Type_MyMusic,

        Type_Group,
        Type_Friend,
        Type_Playlist,
        Type_Search
    };

    QStandardItem* CreateRootItem();
    void LazyPopulate(QStandardItem *parent);
    void ShowContextMenu(const QPoint &global_pos);
    void ItemDoubleClicked(QStandardItem *item);

public slots:
    void Login();
    void Logout();
    
private slots:
    void ShowConfig();

private:
    QStandardItem* CreateStandartItem();
    QStandardItem* need_login_;
    QStandardItem* root_item_;
    QStandardItem* recommendations_;
    QStandardItem* my_music_;
    QVector<QStandardItem*> playlists_;

    boost::scoped_ptr<QMenu> context_menu_;

    Vreen::Client client_;
    bool connected_;
};

#endif // VKSERVICE_H
