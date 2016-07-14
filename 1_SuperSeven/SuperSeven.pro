HEADERS += \
    initShaders.h \
    myDataStructures.h \
    readers.h \
    glm.h

SOURCES += \
    draw3D.cpp \
    initShaders.cpp \
    readers.cpp \
    glm.cpp

LIBS += -lGL -lglut -lGLEW

DISTFILES += \
    axisShader.frag \
    axisShader.vert \
    malha.obj

install_it.path = $$OUT_PWD
install_it.files += axisShader.frag
install_it.files += axisShader.vert
install_it.files += malha.obj

INSTALLS += \
    install_it
