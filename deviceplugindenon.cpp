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

#include "deviceplugindenon.h"
#include "plugininfo.h"

// The constructor of this device plugin.
DevicePluginDenon::DevicePluginDenon()
{
}

DeviceManager::HardwareResources DevicePluginDenon::requiredHardware() const
{
    return DeviceManager::HardwareResourceTimer;
}

DeviceManager::DeviceSetupStatus DevicePluginDenon::setupDevice(Device *device)
{
    qCDebug(dcDenon) << "Setup Denon device" << device->paramValue("ip").toString();

    m_connection = new DenonConnection(QHostAddress(device->paramValue("ip").toString()), 23, this);
    connect (m_connection, SIGNAL(connectionStatusChanged()), this, SLOT(onConnectionChanged()));

    //m_denons.insert(m_connection, device);
    m_asyncSetups.append(m_connection);

    m_connection->connectDenon();
    m_denon.insert(m_connection, device);

    return DeviceManager::DeviceSetupStatusAsync;
}

void DevicePluginDenon::deviceRemoved(Device *device)
{
    qCDebug(dcDenon) << "Delete " << device->name();
}

void DevicePluginDenon::networkManagerReplyReady(QNetworkReply *reply)
{
    Q_UNUSED(reply)
}

void DevicePluginDenon::guhTimer()
{
    /*
    foreach (Denon *denon, m_denons.keys()) {
        if (!denon->connected()) {
            denon->connectDenon();
            continue;
        } else {
            // no need for polling information, notifications do the job
            //denon->update();
        }
    }*/
}

void DevicePluginDenon::actionDataReady(const ActionId &actionId, const QByteArray &data)
{
    Q_UNUSED(actionId)
    Q_UNUSED(data)
}


// This method will be called whenever a client or the rule engine wants to execute an action for the given device.
DeviceManager::DeviceError DevicePluginDenon::executeAction(Device *device, const Action &action)
{
    qCDebug(dcDenon) << "Execute action" << device->id() << action.id() << action.params();

    if (device->deviceClassId() == AVRX1000DeviceClassId) {
        DenonConnection *denon = m_denon.key(device);

        // check connection state
        if (!denon->connected()) {
            return DeviceManager::DeviceErrorHardwareNotAvailable;
        }


        // check if the requested action is our "update" action ...
        if (action.actionTypeId() == powerActionTypeId) {

            // Print information that we are executing now the update action
            qCDebug(dcDenon) << "set power action" << action.id();
            qCDebug(dcDenon) << "power: " << action.param("power").value().Bool;

            if (action.param("power").value().Bool == true){
                denon->sendData("PWON\r");
            } else {
                denon->sendData("PWSTANDBY\r");
            }

            // Tell the DeviceManager that this is an async action and the result of the execution will
            // be emitted later.
            return DeviceManager::DeviceErrorAsync;
        } else if (action.actionTypeId() == volumeActionTypeId) {

            QByteArray vol = action.param("volume").value().toByteArray();
            QByteArray cmd = "MV" + vol + "\r";

            qCDebug(dcDenon) << "Execute volume" << action.id() << cmd;
            denon->sendData(cmd);

        } else if (action.actionTypeId() == changechannelActionTypeId) {

            qCDebug(dcDenon) << "Execute update action" << action.id();

            QByteArray channel = action.param("channel").value().toByteArray();
            QByteArray cmd = "MS" + channel + "\r";

            qCDebug(dcDenon) << "Change to channel:" << cmd;
            denon->sendData(cmd);

        }


        // ...otherwise the ActionType does not exist
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}


void DevicePluginDenon::onConnectionChanged()
{



}






