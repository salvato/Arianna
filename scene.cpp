﻿/****************************************************************************
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
#include "twosidedgraphicswidget.h"

#include <QMatrix4x4>
#include <QRandomGenerator>
#include <QVector3D>
#include <qmath.h>
#include <QNetworkDatagram>

#include "3rdparty/fbm.h"


//============================================================================//
//                                    Scene                                   //
//============================================================================//

const static char
environmentShaderText[] =
    "uniform samplerCube env;"
    "void main() {"
        "gl_FragColor = textureCube(env, gl_TexCoord[1].xyz);"
    "}";

Scene::Scene(int width, int height, int maxTextureSize)
    : m_distExp(600)
    , m_frame(0)
    , m_maxTextureSize(maxTextureSize)
    , m_currentShader(0)
    , m_currentTexture(0)
    , m_dynamicCubemap(false)
    , m_updateAllCubemaps(true)
    , m_box(nullptr)
    , m_vertexShader(nullptr)
    , m_environmentShader(nullptr)
    , m_environmentProgram(nullptr)
    , udpPort(3333)

{
    setSceneRect(0, 0, width, height);
    nTextures = 0;

    m_trackBalls[0] = TrackBall(0.05f,  QVector3D(0, 1, 0), TrackBall::Sphere);
    m_trackBalls[1] = TrackBall(0.005f, QVector3D(0, 0, 1), TrackBall::Sphere);
    m_trackBalls[2] = TrackBall(0.0f,   QVector3D(0, 1, 0), TrackBall::Plane);

    m_renderOptions = new RenderOptionsDialog;
    m_renderOptions->move(20, 120);
    m_renderOptions->resize(m_renderOptions->sizeHint());

    connect(m_renderOptions, &RenderOptionsDialog::dynamicCubemapToggled,
            this, &Scene::toggleDynamicCubemap);
    connect(m_renderOptions, &RenderOptionsDialog::colorParameterChanged,
            this, &Scene::setColorParameter);
    connect(m_renderOptions, &RenderOptionsDialog::floatParameterChanged,
            this, &Scene::setFloatParameter);
    connect(m_renderOptions, &RenderOptionsDialog::textureChanged,
            this, &Scene::setTexture);
    connect(m_renderOptions, &RenderOptionsDialog::shaderChanged,
            this, &Scene::setShader);

    m_itemDialog = new ItemDialog;
    connect(m_itemDialog, &ItemDialog::newItemTriggered, this, &Scene::newItem);

    TwoSidedGraphicsWidget *twoSided = new TwoSidedGraphicsWidget(this);
    twoSided->setWidget(0, m_renderOptions);
    twoSided->setWidget(1, m_itemDialog);

    connect(m_renderOptions, &RenderOptionsDialog::doubleClicked,
            twoSided, &TwoSidedGraphicsWidget::flip);
    connect(m_itemDialog, &ItemDialog::doubleClicked, twoSided,
            &TwoSidedGraphicsWidget::flip);

    initGL();

    m_timer = new QTimer(this);
    m_timer->setInterval(20);
    connect(m_timer, &QTimer::timeout,
            this, [this](){ update(); });
    m_timer->start();

    // Network UDP event listener
    pUdpSocket = new QUdpSocket(this);
    if(!pUdpSocket->bind(QHostAddress::Any, udpPort)) {
        qDebug() << QString("Unable to bind... EXITING");
        exit(-1);
    }
    connect(pUdpSocket, SIGNAL(readyRead()),
            this, SLOT(onReadPendingDatagrams()));

    // Timer to Change Texture
    connect(&timerTexture, SIGNAL(timeout()),
            this, SLOT(onChangeTexture()));
    timerTexture.start(30000);
}


Scene::~Scene() {
    delete m_box;
    qDeleteAll(m_textures);
    delete m_mainCubemap;
    qDeleteAll(m_programs);
    delete m_vertexShader;
    qDeleteAll(m_fragmentShaders);
    qDeleteAll(m_cubemaps);
    delete m_environmentShader;
    delete m_environmentProgram;
}


void
Scene::initGL() {
    m_box = new GLRoundedBox(0.25f, 1.0f, 10);
    m_vertexShader = new QGLShader(QGLShader::Vertex);
    m_vertexShader->compileSourceFile(QLatin1String(":/res/boxes/basic.vsh"));
    QStringList list;
    list << ":/res/boxes/cubemap_posx.jpg"
         << ":/res/boxes/cubemap_negx.jpg"
         << ":/res/boxes/cubemap_posy.jpg"
         << ":/res/boxes/cubemap_negy.jpg"
         << ":/res/boxes/cubemap_posz.jpg"
         << ":/res/boxes/cubemap_negz.jpg";
    m_environment = new GLTextureCube(list, qMin(1024, m_maxTextureSize));
    m_environmentShader = new QGLShader(QGLShader::Fragment);
    m_environmentShader->compileSourceCode(environmentShaderText);
    m_environmentProgram = new QGLShaderProgram;
    m_environmentProgram->addShader(m_vertexShader);
    m_environmentProgram->addShader(m_environmentShader);
    m_environmentProgram->link();
    const int NOISE_SIZE = 128; // for a different size, B and BM in fbm.c must also be changed
    m_noise = new GLTexture3D(NOISE_SIZE, NOISE_SIZE, NOISE_SIZE);
    QVector<QRgb> data(NOISE_SIZE * NOISE_SIZE * NOISE_SIZE, QRgb(0));
    QRgb *p = data.data();
    float pos[3];
    for (int k = 0; k < NOISE_SIZE; ++k) {
        pos[2] = k * (0x20 / (float)NOISE_SIZE);
        for (int j = 0; j < NOISE_SIZE; ++j) {
            for (int i = 0; i < NOISE_SIZE; ++i) {
                for (int byte = 0; byte < 4; ++byte) {
                    pos[0] = (i + (byte & 1) * 16) * (0x20 / (float)NOISE_SIZE);
                    pos[1] = (j + (byte & 2) * 8) * (0x20 / (float)NOISE_SIZE);
                    *p |= (int)(128.0f * (noise3(pos) + 1.0f)) << (byte * 8);
                }
                ++p;
            }
        }
    }
    m_noise->load(NOISE_SIZE, NOISE_SIZE, NOISE_SIZE, data.data());
    m_mainCubemap = new GLRenderTargetCube(512);
    QList<QFileInfo> files;
    // Load all .png files as textures
    m_currentTexture = 0;
    files = QDir(":/res/boxes/").entryInfoList({ QStringLiteral("*.png") }, QDir::Files | QDir::Readable);
    for (const QFileInfo &file : qAsConst(files)) {
        GLTexture *texture = new GLTexture2D(file.absoluteFilePath(), qMin(256, m_maxTextureSize), qMin(256, m_maxTextureSize));
        if (texture->failed()) {
            delete texture;
            continue;
        }
        m_textures << texture;
        m_renderOptions->addTexture(file.baseName());
    }
    nTextures = m_textures.size();
    currentTexture = 0;
    if (m_textures.size() == 0)
        m_textures << new GLTexture2D(qMin(64, m_maxTextureSize), qMin(64, m_maxTextureSize));
    // Load all .fsh files as fragment shaders
    m_currentShader = 0;
    files = QDir(":/res/boxes/").entryInfoList({ QStringLiteral("*.fsh") }, QDir::Files | QDir::Readable);
    for (const QFileInfo &file : qAsConst(files)) {
        QGLShaderProgram *program = new QGLShaderProgram;
        QGLShader* shader = new QGLShader(QGLShader::Fragment);
        shader->compileSourceFile(file.absoluteFilePath());
        // The program does not take ownership over the shaders, so store them in a vector so they can be deleted afterwards.
        program->addShader(m_vertexShader);
        program->addShader(shader);
        if (!program->link()) {
            qWarning("Failed to compile and link shader program");
            qWarning("Vertex shader log:");
            qWarning() << m_vertexShader->log();
            qWarning() << "Fragment shader log ( file =" << file.absoluteFilePath() << "):";
            qWarning() << shader->log();
            qWarning("Shader program log:");
            qWarning() << program->log();
            delete shader;
            delete program;
            continue;
        }
        m_fragmentShaders << shader;
        m_programs << program;
        m_renderOptions->addShader(file.baseName());
        program->bind();
        m_cubemaps << ((program->uniformLocation("env") != -1) ? new GLRenderTargetCube(qMin(256, m_maxTextureSize)) : nullptr);
        program->release();
    }
    if (m_programs.size() == 0)
        m_programs << new QGLShaderProgram;
    m_renderOptions->emitParameterChanged();
}


static void
loadMatrix(const QMatrix4x4 &m) {
    // static to prevent glLoadMatrixf to fail on certain drivers
    static GLfloat mat[16];
    const float *data = m.constData();
    for (int index = 0; index < 16; ++index)
        mat[index] = data[index];
    glLoadMatrixf(mat);
}


// If one of the boxes should not be rendered, set excludeBox to its index.
// If the main box should not be rendered, set excludeBox to -1.
void
Scene::renderBoxes(const QMatrix4x4 &view, int excludeBox) {
    QMatrix4x4 invView = view.inverted();
    // If multi-texturing is supported, use three saplers.
    if (glActiveTexture) {
        glActiveTexture(GL_TEXTURE0);
        m_textures[m_currentTexture]->bind();
        glActiveTexture(GL_TEXTURE2);
        m_noise->bind();
        glActiveTexture(GL_TEXTURE1);
    } else {
        m_textures[m_currentTexture]->bind();
    }
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    QMatrix4x4 viewRotation(view);
    viewRotation(3, 0) = viewRotation(3, 1) = viewRotation(3, 2) = 0.0f;
    viewRotation(0, 3) = viewRotation(1, 3) = viewRotation(2, 3) = 0.0f;
    viewRotation(3, 3) = 1.0f;
    loadMatrix(viewRotation);
    glScalef(20.0f, 20.0f, 20.0f);
    // Don't render the environment if the environment texture can't be set for the correct sampler.
    if (glActiveTexture) {
        m_environment->bind();
        m_environmentProgram->bind();
        m_environmentProgram->setUniformValue("tex", GLint(0));
        m_environmentProgram->setUniformValue("env", GLint(1));
        m_environmentProgram->setUniformValue("noise", GLint(2));
        m_box->draw();
        m_environmentProgram->release();
        m_environment->unbind();
    }
    loadMatrix(view);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    for (int i = 0; i < m_programs.size(); ++i) {
        if (i == excludeBox)
            continue;
        glPushMatrix();
        QMatrix4x4 m;
        m.rotate(m_trackBalls[1].rotation());
        glMultMatrixf(m.constData());
        glRotatef(360.0f * i / m_programs.size(), 0.0f, 0.0f, 1.0f);
        glTranslatef(2.0f, 0.0f, 0.0f);
        glScalef(0.3f, 0.6f, 0.6f);

        if (glActiveTexture) {
            if (m_dynamicCubemap && m_cubemaps[i])
                m_cubemaps[i]->bind();
            else
                m_environment->bind();
        }
        m_programs[i]->bind();
        m_programs[i]->setUniformValue("tex", GLint(0));
        m_programs[i]->setUniformValue("env", GLint(1));
        m_programs[i]->setUniformValue("noise", GLint(2));
        m_programs[i]->setUniformValue("view", view);
        m_programs[i]->setUniformValue("invView", invView);
        m_box->draw();
        m_programs[i]->release();
        if (glActiveTexture) {
            if (m_dynamicCubemap && m_cubemaps[i])
                m_cubemaps[i]->unbind();
            else
                m_environment->unbind();
        }
        glPopMatrix();
    }
    if (-1 != excludeBox) {
        QMatrix4x4 m;
        m.rotate(QQuaternion(q0, q1, q2, q3));
        glMultMatrixf(m.constData());
        if (glActiveTexture) {
            if (m_dynamicCubemap)
                m_mainCubemap->bind();
            else
                m_environment->bind();
        }
        m_programs[m_currentShader]->bind();
        m_programs[m_currentShader]->setUniformValue("tex", GLint(0));
        m_programs[m_currentShader]->setUniformValue("env", GLint(1));
        m_programs[m_currentShader]->setUniformValue("noise", GLint(2));
        m_programs[m_currentShader]->setUniformValue("view", view);
        m_programs[m_currentShader]->setUniformValue("invView", invView);
        m_box->draw();
        m_programs[m_currentShader]->release();
        if (glActiveTexture) {
            if (m_dynamicCubemap)
                m_mainCubemap->unbind();
            else
                m_environment->unbind();
        }
    }
    if (glActiveTexture) {
        glActiveTexture(GL_TEXTURE2);
        m_noise->unbind();
        glActiveTexture(GL_TEXTURE0);
    }
    m_textures[m_currentTexture]->unbind();
}


void
Scene::setStates() {
    //glClearColor(0.25f, 0.25f, 0.5f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    //glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_NORMALIZE);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    setLights();
    float materialSpecular[] = {0.5f, 0.5f, 0.5f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);
}


void
Scene::setLights() {
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    //float lightColour[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float lightDir[] = {0.0f, 0.0f, 1.0f, 0.0f};
    //glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColour);
    //glLightfv(GL_LIGHT0, GL_SPECULAR, lightColour);
    glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
    glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0f);
    glEnable(GL_LIGHT0);
}


void
Scene::defaultStates() {
    //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    //glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHT0);
    glDisable(GL_NORMALIZE);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 0.0f);
    float defaultMaterialSpecular[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defaultMaterialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
}


void
Scene::renderCubemaps() {
    // To speed things up, only update the cubemaps for the small cubes every N frames.
    const int N = (m_updateAllCubemaps ? 1 : 3);
    QMatrix4x4 mat;
    GLRenderTargetCube::getProjectionMatrix(mat, 0.1f, 100.0f);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    loadMatrix(mat);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    QVector3D center;
    const float eachAngle = 2 * M_PI / m_cubemaps.size();
    for (int i = m_frame % N; i < m_cubemaps.size(); i += N) {
        if (0 == m_cubemaps[i])
            continue;
        float angle = i * eachAngle;
        center = m_trackBalls[1].rotation().rotatedVector(QVector3D(std::cos(angle), std::sin(angle), 0.0f));
        for (int face = 0; face < 6; ++face) {
            m_cubemaps[i]->begin(face);
            GLRenderTargetCube::getViewMatrix(mat, face);
            QVector4D v = QVector4D(-center.x(), -center.y(), -center.z(), 1.0);
            mat.setColumn(3, mat * v);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderBoxes(mat, i);
            m_cubemaps[i]->end();
        }
    }
    for (int face = 0; face < 6; ++face) {
        m_mainCubemap->begin(face);
        GLRenderTargetCube::getViewMatrix(mat, face);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderBoxes(mat, -1);
        m_mainCubemap->end();
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    m_updateAllCubemaps = false;
}


void
Scene::drawBackground(QPainter *painter, const QRectF &) {
    float width = float(painter->device()->width());
    float height = float(painter->device()->height());
    painter->beginNativePainting();
    setStates();
    if (m_dynamicCubemap)
        renderCubemaps();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    qgluPerspective(60.0, width / height, 0.01, 15.0);
    glMatrixMode(GL_MODELVIEW);
    QMatrix4x4 view;
    view.rotate(m_trackBalls[2].rotation());
    view(2, 3) -= 2.0f * std::exp(m_distExp / 1200.0f);
    renderBoxes(view);
    defaultStates();
    ++m_frame;
    painter->endNativePainting();
}


QPointF
Scene::pixelPosToViewPos(const QPointF& p) {
    return QPointF(2.0 * float(p.x()) / width() - 1.0,
                   1.0 - 2.0 * float(p.y()) / height());
}


void
Scene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsScene::mouseMoveEvent(event);
    if (event->isAccepted())
        return;
    if (event->buttons() & Qt::LeftButton) {
        m_trackBalls[0].move(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugated());
        event->accept();
    } else {
        m_trackBalls[0].release(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugated());
    }
    if (event->buttons() & Qt::RightButton) {
        m_trackBalls[1].move(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugated());
        event->accept();
    } else {
        m_trackBalls[1].release(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugated());
    }
    if (event->buttons() & Qt::MidButton) {
        m_trackBalls[2].move(pixelPosToViewPos(event->scenePos()), QQuaternion());
        event->accept();
    } else {
        m_trackBalls[2].release(pixelPosToViewPos(event->scenePos()), QQuaternion());
    }
}


void
Scene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsScene::mousePressEvent(event);
    if (event->isAccepted())
        return;
    if (event->buttons() & Qt::LeftButton) {
        m_trackBalls[0].push(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugated());
        event->accept();
    }
    if (event->buttons() & Qt::RightButton) {
        m_trackBalls[1].push(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugated());
        event->accept();
    }
    if (event->buttons() & Qt::MidButton) {
        m_trackBalls[2].push(pixelPosToViewPos(event->scenePos()), QQuaternion());
        event->accept();
    }
}


void
Scene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsScene::mouseReleaseEvent(event);
    if (event->isAccepted())
        return;
    if (event->button() == Qt::LeftButton) {
        m_trackBalls[0].release(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugated());
        event->accept();
    }
    if (event->button() == Qt::RightButton) {
        m_trackBalls[1].release(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugated());
        event->accept();
    }
    if (event->button() == Qt::MidButton) {
        m_trackBalls[2].release(pixelPosToViewPos(event->scenePos()), QQuaternion());
        event->accept();
    }
}


void
Scene::wheelEvent(QGraphicsSceneWheelEvent * event) {
    QGraphicsScene::wheelEvent(event);
    if(!event->isAccepted()) {
        m_distExp += event->delta();
        if(m_distExp < -8 * 120)
            m_distExp = -8 * 120;
        if(m_distExp > 10 * 120)
            m_distExp = 10 * 120;
        event->accept();
    }
}


void
Scene::setShader(int index) {
    if (index >= 0 && index < m_fragmentShaders.size())
        m_currentShader = index;
}


void
Scene::setTexture(int index) {
    if (index >= 0 && index < m_textures.size())
        m_currentTexture = index;
}


void
Scene::toggleDynamicCubemap(int state) {
    if ((m_dynamicCubemap = (state == Qt::Checked)))
        m_updateAllCubemaps = true;
}


void
Scene::setColorParameter(const QString &name, QRgb color) {
    // set the color in all programs
    for (QGLShaderProgram *program : qAsConst(m_programs)) {
        program->bind();
        program->setUniformValue(program->uniformLocation(name), QColor(color));
        program->release();
    }
}


void
Scene::setFloatParameter(const QString &name, float value) {
    // set the color in all programs
    for (QGLShaderProgram *program : qAsConst(m_programs)) {
        program->bind();
        program->setUniformValue(program->uniformLocation(name), value);
        program->release();
    }
}


void
Scene::newItem(ItemDialog::ItemType type) {
    QSize size = sceneRect().size().toSize();
    switch (type) {
    case ItemDialog::QtBoxItem:
        addItem(new QtBox(64, QRandomGenerator::global()->bounded(size.width() - 64) + 32,
                          QRandomGenerator::global()->bounded(size.height() - 64) + 32));
        break;
    case ItemDialog::CircleItem:
        addItem(new CircleItem(64, QRandomGenerator::global()->bounded(size.width() - 64) + 32,
                               QRandomGenerator::global()->bounded(size.height() - 64) + 32));
        break;
    case ItemDialog::SquareItem:
        addItem(new SquareItem(64, QRandomGenerator::global()->bounded(size.width() - 64) + 32,
                               QRandomGenerator::global()->bounded(size.height() - 64) + 32));
        break;
    default:
        break;
    }
}


void
Scene::onReadPendingDatagrams() {
    while(pUdpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = pUdpSocket->receiveDatagram();
        QByteArray received = datagram.data();
        if(received.size() != 4*sizeof(float))
            qDebug() << "Size differs";
        else {
            memcpy(&q0, received.constData(),    4);
            memcpy(&q1, received.constData()+4,  4);
            memcpy(&q2, received.constData()+8,  4);
            memcpy(&q3, received.constData()+12, 4);
        }
    }
}


void
Scene::onChangeTexture() {
    currentTexture += 1;
    if(currentTexture >= nTextures)
        currentTexture = 0;
    setTexture(currentTexture);
}
