/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stuerz <simon.stuerz@guh.guru>                *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DEVICEPLUGINDENON_H
#define DEVICEPLUGINDENON_H

#include "devicemanager.h"
#include "plugin/deviceplugin.h"

#include <QHash>
#include <QNetworkReply>
#include <QObject>
#include <QHostAddress>

#include "denonconnection.h"


class DevicePluginDenon : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "guru.guh.DevicePlugin" FILE "deviceplugindenon.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginDenon();

    DeviceManager::HardwareResources requiredHardware() const override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

    void networkManagerReplyReady(QNetworkReply *reply) override;

    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

    void guhTimer();
private:
    QHash <ActionId, Device *> m_asyncActions;
    QHash <QNetworkReply *, ActionId> m_asyncActionReplies;


    DenonConnection *m_connection;
    QHash<DenonConnection *, Device *> m_denon;
    QList<DenonConnection *> m_asyncSetups;


    void actionDataReady(const ActionId &actionId, const QByteArray &data);

private slots:
    void onConnectionChanged();
    void onStateChanged();
    void onActionExecuted(const ActionId &actionId, const bool &success);
    void versionDataReceived(const QVariantMap &data);
    void onSetupFinished(const QVariantMap &data);
};

#endif // DEVICEPLUGINDENON_H
