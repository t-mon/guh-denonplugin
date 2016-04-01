
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stuerz <simon.stuerz@guh.guru>                *
 *  Copyright (C) 2016 Bernhard Trinnes <bernhard.trinnes@guh.guru         *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "denonconnection.h"
#include "extern-plugininfo.h"

#include <QPixmap>

DenonConnection::DenonConnection(const QHostAddress &hostAddress, const int &port, QObject *parent) :
    QObject(parent),
    m_hostAddress(hostAddress),
    m_port(port),
    m_connected(false)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &DenonConnection::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &DenonConnection::onDisconnected);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
    connect(m_socket, &QTcpSocket::readyRead, this, &DenonConnection::readData);
}

void DenonConnection::connectDenon()
{
    if (m_socket->state() == QAbstractSocket::ConnectingState) {
        return;
    }
    m_socket->connectToHost(m_hostAddress, m_port);
}

void DenonConnection::disconnectDenon()
{
    m_socket->close();
}

QHostAddress DenonConnection::hostAddress() const
{
    return m_hostAddress;
}

int DenonConnection::port() const
{
    return m_port;
}

bool DenonConnection::connected()
{
    return m_connected;
}

void DenonConnection::onConnected()
{
    qCDebug(dcDenon) << "connected successfully to" << hostAddress().toString() << port();
    m_connected = true;
    emit connectionStatusChanged();
}

void DenonConnection::onDisconnected()
{
    qCDebug(dcDenon) << "disconnected from" << hostAddress().toString() << port();
    m_connected = false;
    emit connectionStatusChanged();
}

void DenonConnection::onError(QAbstractSocket::SocketError socketError)
{
    if (connected()) {
        qCWarning(dcDenon) << "socket error:" << socketError << m_socket->errorString();
    }
}

void DenonConnection::readData()
{
    QByteArray data = m_socket->readAll();

    QStringList commandList = QString(data).split("}{");
    for(int i = 0; i < commandList.count(); ++i) {
        QString command = commandList.at(i);
        if(command.isEmpty()) {
            continue;
        }
        if(i < commandList.count() - 1) {
            command.append("}");
        }
        if(i > 0) {
            command.prepend("{");
        }
        emit dataReady(command.toUtf8());
    }
}

void DenonConnection::sendData(const QByteArray &message)
{
    m_socket->write(message);
}
