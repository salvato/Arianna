/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "graphicsview.h"
#include <QNetworkDatagram>


GraphicsView::GraphicsView() {
    setWindowTitle(tr("Boxes"));
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
/*
    udpPort = 37755;
    pUdpSocket = new QUdpSocket(this);
    if(!pUdpSocket->bind(QHostAddress::Any, udpPort)) {
        qDebug() << QString("Unable to bind... EXITING");
        exit(-1);
    }

    // Network UDP events
    connect(pUdpSocket, SIGNAL(readyRead()),
            this, SLOT(onReadPendingDatagrams()));
*/
}


void
GraphicsView::resizeEvent(QResizeEvent *event) {
    if(scene())
        scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
    QGraphicsView::resizeEvent(event);
}


void
GraphicsView::executeCommand(QString command) {
/*
    QStringList tokens = command.split(' ');
    tokens.removeFirst();
    char cmd = command.at(0).toLatin1();
    if(cmd == 'q') { // It is a Quaternion !
        if(tokens.count() == 4) {
            q0 = tokens.at(0).toDouble();
            q1 = tokens.at(1).toDouble();
            q2 = tokens.at(2).toDouble();
            q3 = tokens.at(3).toDouble();
            pGLWidget->setRotation(q0, q1, q2, q3);
        }
    }
*/
}


void
GraphicsView::onReadPendingDatagrams() {
/*
    while(pUdpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = pUdpSocket->receiveDatagram();
        QString sReceived = QString(datagram.data());
        QString sNewCommand;
        int iPos;
        iPos = sReceived.indexOf("#");
        while(iPos != -1) {
            sNewCommand = sReceived.left(iPos);
            executeCommand(sNewCommand);
            sReceived = sReceived.mid(iPos+1);
            iPos = sReceived.indexOf("#");
        }
    }
*/
}

