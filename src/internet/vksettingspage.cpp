#include "vksettingspage.h"

#include "ui_vksettingspage.h"
#include "core/application.h"
#include "core/logging.h"
#include "internet/vkservice.h"


VkSettingsPage::VkSettingsPage(SettingsDialog *parent)
    : SettingsPage(parent),
      ui_(new Ui::VkSettingsPage),
      service_(dialog()->app()->internet_model()->Service<VkService>())
{
    ui_->setupUi(this);
    connect(service_, SIGNAL(LoginSuccess(bool)),
            SLOT(LoginSuccess(bool)));
}

VkSettingsPage::~VkSettingsPage()
{
    delete ui_;
}

void VkSettingsPage::Load()
{
    QSettings s;
    s.beginGroup(VkService::kSettingGroup);

    if (service_->hasAccount()) {
        Login();
    } else {
        Logout();
    }
}

void VkSettingsPage::Save()
{
}

void VkSettingsPage::Login()
{
    qLog(Debug) << "Login clicked";
    ui_->account->setEnabled(false);
    service_->Login();
}

void VkSettingsPage::LoginSuccess(bool succ)
{
    qLog(Debug) << "LoginSuccess" << succ;
    if (succ) {
        ui_->login_button->setText("Logout");

        ui_->name->setText("Loading...");
        connect(service_, SIGNAL(NameUpdated(QString)),
                ui_->name, SLOT(setText(QString)));

        connect(ui_->login_button, SIGNAL(clicked()),
                SLOT(Logout()));
        disconnect(ui_->login_button, SIGNAL(clicked()),
                   this, SLOT(Login()));
    }
    ui_->account->setEnabled(true);
    qLog(Debug) << " DONE LoginSuccess" << succ;
}

void VkSettingsPage::Logout()
{
    qLog(Debug) << "Logout clicked";
    service_->Logout();

    ui_->login_button->setText("Login");
    ui_->name->setText("");

    connect(ui_->login_button, SIGNAL(clicked()),
            SLOT(Login()));
    disconnect(ui_->login_button, SIGNAL(clicked()),
               this, SLOT(Logout()));
    ui_->account->setEnabled(true);
}

