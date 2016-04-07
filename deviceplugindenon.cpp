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
    //connect (m_connection, SIGNAL(connectionStatusChanged()), this, SLOT(onConnectionChanged()));
    connect(m_connection, &DenonConnection::connectionStatusChanged, this, &DevicePluginDenon::onConnectionChanged);
    connect(m_connection, &DenonConnection::connectionStatusChanged, this, &DevicePluginDenon::onSetupFinished);
    //connect(m_connection, &DenonConnection::, this, &DevicePluginDenon::onActionExecuted);
    connect(m_connection, &DenonConnection::dataReady, this, &DevicePluginDenon::onDataReceived);

    //m_denons.insert(m_connection, device);
    m_asyncSetups.append(m_connection);

    m_connection->connectDenon();
    m_denon.insert(m_connection, device);

    return DeviceManager::DeviceSetupStatusAsync;
}

DeviceManager::DeviceError DevicePluginDenon::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)
    Q_UNUSED(deviceClassId)
    qCDebug(dcDenon) << "Start UPnP search";
    //upnpDiscover();
    //avahi browse
    return DeviceManager::DeviceErrorAsync;
}

void DevicePluginDenon::avahiDiscoveryFinished()
{
    QList<DeviceDescriptor> deviceDescriptors;
    emit devicesDiscovered(AVRX1000DeviceClassId, deviceDescriptors);
}


void DevicePluginDenon::onSetupFinished()
{
    DenonConnection *denon = static_cast<DenonConnection *>(sender());
    Device *device = m_denon.value(denon);

    denon->sendData("PW?\rSI?\rMV?\r");


    emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);

}


void DevicePluginDenon::deviceRemoved(Device *device)
{
    qCDebug(dcDenon) << "Delete " << device->name();
    if (!m_denon.values().contains(device))
        return;

    QPointer<DenonConnection> denon = m_denon.key(device);

    if (!denon.isNull()){
        qCWarning(dcDenon) << "Invalid denon pointer" << device->id().toString();
        return;
    }
    m_denon.remove(denon);
    qCDebug(dcDenon) << "Delete " << device->name();
    denon->deleteLater();
}


void DevicePluginDenon::guhTimer()
{

    foreach (DenonConnection *denon, m_denon.keys()) {
        if (!denon->connected()) {
            denon->connectDenon();
            continue;
        } else {
            denon->sendData("PW?\rSI?\rMV?\r");
        }
    }
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

            if (action.param("power").value().toBool() == true){
                QByteArray cmd = "PWON\r";
                qCDebug(dcDenon) << "Execute power: " << action.id() << cmd;
                denon->sendData(cmd);
            } else {
                QByteArray cmd = "PWSTANDBY\r";
                qCDebug(dcDenon) << "Execute power: " << action.id() << cmd;
                denon->sendData(cmd);
            }

            // Tell the DeviceManager that this is an async action and the result of the execution will
            // be emitted later.
            return DeviceManager::DeviceErrorNoError;
        } else if (action.actionTypeId() == volumeActionTypeId) {

            QByteArray vol = action.param("volume").value().toByteArray();
            QByteArray cmd = "MV" + vol + "\r";

            qCDebug(dcDenon) << "Execute volume" << action.id() << cmd;
            denon->sendData(cmd);

            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == channelActionTypeId) {

            qCDebug(dcDenon) << "Execute update action" << action.id();

            QByteArray channel = action.param("channel").value().toByteArray();
            QByteArray cmd = "SI" + channel + "\r";

            qCDebug(dcDenon) << "Change to channel:" << cmd;
            denon->sendData(cmd);

            return DeviceManager::DeviceErrorNoError;

        }

        // ...otherwise the ActionType does not exist
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginDenon::onActionExecuted(const ActionId &actionId, const bool &success)
{
    if (success) {
        emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorNoError);
    } else {
        emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorInvalidParameter);
    }
}


void DevicePluginDenon::onConnectionChanged()
{
    DenonConnection *denon = static_cast<DenonConnection *>(sender());
    Device *device = m_denon.value(denon);

    if (denon->connected()) {

        // if this is the first setup, check version
        if (m_asyncSetups.contains(denon)) {
            m_asyncSetups.removeAll(denon);
        }
    }
    device->setStateValue(connectedStateTypeId, denon->connected());
}

void DevicePluginDenon::onDataReceived(const QByteArray &data){

    DenonConnection *denon = static_cast<DenonConnection *>(sender());
    Device *device = m_denon.value(denon);

    qDebug(dcDenon) << "Data received" << data;

    if (data.contains("MV") && !data.contains("MAX")){
        int index = data.indexOf("MV");
        int vol = data.mid(index+2, 2).toInt();

        qDebug(dcDenon) << "update volume:" << vol;
        device->setStateValue(volumeStateTypeId, vol);
    }

    if(data.contains("SI")){

        QString cmd = NULL;
        if(data.contains("TUNER")){
            cmd = "TUNER";
        } else if(data.contains("DVD")){
            cmd = "DVD";
        } else if(data.contains("BD")){
            cmd = "BD";
        } else if(data.contains("TV")){
            cmd = "TV";
        } else if(data.contains("SAT/CBL")){
            cmd = "SAT/CBL";
        } else if(data.contains("MPLAY")){
            cmd = "MPLAY";
        } else if(data.contains("GAME")){
            cmd = "GAME";
        } else if(data.contains("AUX1")){
            cmd = "AUX1";
        } else if(data.contains("NET")){
            cmd = "NET";
        } else if(data.contains("PANDORA")){
            cmd = "PANDORA";
        } else if(data.contains("SIRIUSXM")){
            cmd = "SIRIUSXM";
        } else if(data.contains("SPOTIFY")){
            cmd = "SPOTIFY";
        } else if(data.contains("FLICKR")){
            cmd = "FLICKR";
        } else if(data.contains("FAVORITES")){
            cmd = "FAVORITES";
        } else if(data.contains("IRADIO")){
            cmd = "IRADIO";
        } else if(data.contains("SERVER")){
            cmd = "SERVER";
        } else if(data.contains("USB/IPOD")){
            cmd = "USB/IPOD";
        } else if(data.contains("IPD")){
            cmd = "IPD";
        } else if(data.contains("IRP")){
            cmd = "IRP";
        } else if(data.contains("FVP")){
            cmd = "FVP";
        }

        qDebug(dcDenon) << "update channel:" << cmd;
        device->setStateValue(channelStateTypeId, cmd);
    }

    if (data.contains("PWON")){
        qDebug(dcDenon) << "update power on";
        device->setStateValue(powerStateTypeId, true);
    } else if (data.contains("PWSTANDBY")){
        qDebug(dcDenon) << "update power off";
        device->setStateValue(powerStateTypeId, false);
    }
}



