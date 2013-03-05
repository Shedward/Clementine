#include "vksettingspage.h"

#include "ui_vksettingspage.h"
#include "core/application.h"
#include "internet/vkservice.h"


VkSettingsPage::VkSettingsPage(SettingsDialog *parent)
    : SettingsPage(parent),
      ui_(new Ui::VkSettingsPage),
      service_(dialog()->app()->internet_model()->Service<VkService>())
{
    ui_->setupUi(this);
}

VkSettingsPage::~VkSettingsPage()
{
    delete ui_;
}

void VkSettingsPage::Load()
{
    QSettings s;
    s.beginGroup(VkService::kSettingGroup);
    QString name = s.value("name").toString();
    if (name.isEmpty()) {
        ui_->name->setText(
                    tr("Clicking the Login button will "
                       "open a web browser.  You should "
                       "return to Clementine after you "
                       "have logged in."));
        ui_->photo->setPixmap(QPixmap(":vk/deactivated.gif"));
        ui_->login_button->setText("Login");
    } else {
        ui_->name->setText(s.value("name").toString());
        ui_->photo->setPixmap(QPixmap(s.value("photo").toString()));
        ui_->login_button->setText("Logout");
    }
}

void VkSettingsPage::Save()
{
}

void VkSettingsPage::OnlineStateChanged()
{
}

