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

#include "scene.h"

#include <QMatrix4x4>
#include <QRandomGenerator>
#include <QVector3D>
#include <qmath.h>

#include "3rdparty/fbm.h"


//============================================================================//
//                                 ItemDialog                                 //
//============================================================================//

ItemDialog::ItemDialog()
    : QDialog(nullptr, Qt::CustomizeWindowHint | Qt::WindowTitleHint)
{
    setWindowTitle(tr("Items (double click to flip)"));
    setWindowOpacity(0.75);
    resize(160, 100);

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    QPushButton *button;

    button = new QPushButton(tr("Add Qt box"));
    layout->addWidget(button);
    connect(button, &QAbstractButton::clicked, this, &ItemDialog::triggerNewQtBox);

    button = new QPushButton(tr("Add circle"));
    layout->addWidget(button);
    connect(button, &QAbstractButton::clicked, this, &ItemDialog::triggerNewCircleItem);

    button = new QPushButton(tr("Add square"));
    layout->addWidget(button);
    connect(button, &QAbstractButton::clicked, this, &ItemDialog::triggerNewSquareItem);

    layout->addStretch(1);
}

void ItemDialog::triggerNewQtBox()
{
    emit newItemTriggered(QtBoxItem);
}

void ItemDialog::triggerNewCircleItem()
{
    emit newItemTriggered(CircleItem);
}

void ItemDialog::triggerNewSquareItem()
{
    emit newItemTriggered(SquareItem);
}

void ItemDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit doubleClicked();
}
