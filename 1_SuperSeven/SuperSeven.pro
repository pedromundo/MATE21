HEADERS += \
    initShaders.h \
    myDataStructures.h

SOURCES += \
    draw3D.cpp \
    initShaders.cpp

LIBS += -lGL -lglut -lGLEW

DISTFILES += \
    axisShader.frag \
    axisShader.vert

install_it.path = $$OUT_PWD
install_it.files += axisShader.frag
install_it.files += axisShader.vert

INSTALLS += \
    install_it
