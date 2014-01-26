#ifndef VKSEARCHDIALOG_H
#define VKSEARCHDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QTimer>

#include "vkservice.h"

namespace Ui {
class VkSearchDialog;
}

class VkSearchDialog : public QDialog
{
  Q_OBJECT

public:
  explicit VkSearchDialog(VkService *service, QWidget *parent = 0);
  ~VkSearchDialog();
  MusicOwner found() const;

signals:
  void Find(const QString &query);

public slots:
  void ReciveResults(const SearchID &id, const MusicOwnerList &owners);

protected:
  void showEvent(QShowEvent *);

private slots:
  void selectionChanged();
  void suggest();
  void selected();

private:
  bool eventFilter(QObject *obj, QEvent *ev);
  QTreeWidgetItem *createItem(const MusicOwner &own);

  Ui::VkSearchDialog *ui;
  MusicOwner selected_;
  VkService *service_;
  SearchID last_search_;
  QTreeWidget *popup;
  QTimer *timer;
};

#endif // VKSEARCHDIALOG_H
