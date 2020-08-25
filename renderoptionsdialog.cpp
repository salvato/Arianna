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

#include "renderoptionsdialog.h"
#include "coloredit.h"
#include "floatedit.h"


//============================================================================//
//                             RenderOptionsDialog                            //
//============================================================================//

RenderOptionsDialog::RenderOptionsDialog()
    : QDialog(nullptr, Qt::CustomizeWindowHint | Qt::WindowTitleHint)
{
    setWindowOpacity(0.75);
    setWindowTitle(tr("Options (double click to flip)"));
    QGridLayout *layout = new QGridLayout;
    setLayout(layout);
    layout->setColumnStretch(1, 1);

    int row = 0;

    QCheckBox *check = new QCheckBox(tr("Dynamic cube map"));
    check->setCheckState(Qt::Unchecked);
    // Dynamic cube maps are only enabled when multi-texturing and render to texture are available.
    check->setEnabled(glActiveTexture && glGenFramebuffersEXT);
    connect(check, &QCheckBox::stateChanged, this, &RenderOptionsDialog::dynamicCubemapToggled);
    layout->addWidget(check, 0, 0, 1, 2);
    ++row;

    // Load all .par files
    // .par files have a simple syntax for specifying user adjustable uniform variables.
    const QList<QFileInfo> files = QDir(QStringLiteral(":/res/boxes/"))
            .entryInfoList({ QStringLiteral("*.par") },
                           QDir::Files | QDir::Readable);

    for (const QFileInfo &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            while (!file.atEnd()) {
                QList<QByteArray> tokens = file.readLine().simplified().split(' ');
                QList<QByteArray>::const_iterator it = tokens.begin();
                if (it == tokens.end())
                    continue;
                QByteArray type = *it;
                if (++it == tokens.end())
                    continue;
                QByteArray name = *it;
                bool singleElement = (tokens.size() == 3); // type, name and one value
                char counter[10] = "000000000";
                int counterPos = 8; // position of last digit
                while (++it != tokens.end()) {
                    m_parameterNames << name;
                    if (!singleElement) {
                        m_parameterNames.back() += '[';
                        m_parameterNames.back() += counter + counterPos;
                        m_parameterNames.back() += ']';
                        int j = 8; // position of last digit
                        ++counter[j];
                        while (j > 0 && counter[j] > '9') {
                            counter[j] = '0';
                            ++counter[--j];
                        }
                        if (j < counterPos)
                            counterPos = j;
                    }

                    if (type == "color") {
                        layout->addWidget(new QLabel(m_parameterNames.back()));
                        bool ok;
                        ColorEdit *colorEdit = new ColorEdit(it->toUInt(&ok, 16), m_parameterNames.size() - 1);
                        m_parameterEdits << colorEdit;
                        layout->addWidget(colorEdit);
                        connect(colorEdit, &ColorEdit::colorChanged, this, &RenderOptionsDialog::setColorParameter);
                        ++row;
                    } else if (type == "float") {
                        layout->addWidget(new QLabel(m_parameterNames.back()));
                        bool ok;
                        FloatEdit *floatEdit = new FloatEdit(it->toFloat(&ok), m_parameterNames.size() - 1);
                        m_parameterEdits << floatEdit;
                        layout->addWidget(floatEdit);
                        connect(floatEdit, &FloatEdit::valueChanged, this, &RenderOptionsDialog::setFloatParameter);
                        ++row;
                    }
                }
            }
            file.close();
        }
    }

    layout->addWidget(new QLabel(tr("Texture:")));
    m_textureCombo = new QComboBox;
    connect(m_textureCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RenderOptionsDialog::textureChanged);
    layout->addWidget(m_textureCombo);
    ++row;

    layout->addWidget(new QLabel(tr("Shader:")));
    m_shaderCombo = new QComboBox;
    connect(m_shaderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RenderOptionsDialog::shaderChanged);
    layout->addWidget(m_shaderCombo);
    ++row;

    layout->setRowStretch(row, 1);
}

int RenderOptionsDialog::addTexture(const QString &name)
{
    m_textureCombo->addItem(name);
    return m_textureCombo->count() - 1;
}

int RenderOptionsDialog::addShader(const QString &name)
{
    m_shaderCombo->addItem(name);
    return m_shaderCombo->count() - 1;
}

void RenderOptionsDialog::emitParameterChanged()
{
    for (ParameterEdit *edit : qAsConst(m_parameterEdits))
        edit->emitChange();
}

void RenderOptionsDialog::setColorParameter(QRgb color, int id)
{
    emit colorParameterChanged(m_parameterNames[id], color);
}

void RenderOptionsDialog::setFloatParameter(float value, int id)
{
    emit floatParameterChanged(m_parameterNames[id], value);
}

void RenderOptionsDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit doubleClicked();
}

