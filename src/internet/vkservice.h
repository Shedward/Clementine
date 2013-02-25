#ifndef VKSERVICE_H
#define VKSERVICE_H

#include "internetservice.h"

class VkService : public InternetService
{
    Q_OBJECT
public:
    explicit VkService(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // VKSERVICE_H
