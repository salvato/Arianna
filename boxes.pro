QT += opengl
QT += widgets
QT += multimedia


requires(qtConfig(combobox))


qtConfig(opengles.|angle|dynamicgl): error("This example requires Qt to be configured with -opengl desktop")


HEADERS += 3rdparty/fbm.h \
           coloredit.h \
           floatedit.h \
           glbuffers.h \
           glextensions.h \
           gltrianglemesh.h \
           graphicsview.h \
           graphicswidget.h \
           itemdialog.h \
           qtbox.h \
           renderoptionsdialog.h \
           roundedbox.h \
           scene.h \
           trackball.h \
           twosidedgraphicswidget.h

SOURCES += 3rdparty/fbm.c \
           coloredit.cpp \
           floatedit.cpp \
           glbuffers.cpp \
           glextensions.cpp \
           graphicsview.cpp \
           graphicswidget.cpp \
           itemdialog.cpp \
           main.cpp \
           qtbox.cpp \
           renderoptionsdialog.cpp \
           roundedbox.cpp \
           scene.cpp \
           trackball.cpp \
           twosidedgraphicswidget.cpp

RESOURCES += boxes.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/boxes
INSTALLS += target
