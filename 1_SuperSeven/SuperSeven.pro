HEADERS += \
    initShaders.h \
    myDataStructures.h \
    readers.h \
    glm.h \
    hello-world.h \
    dev_array.h

SOURCES += \
    draw3D.cpp \
    initShaders.cpp \
    readers.cpp \
    glm.cpp

LIBS += -lGL -lglut -lGLEW -lcudart -lcudadevrt -lcuda

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

#  CUDA

CUDA_SOURCES = hello-world.cu

# Flags used by the CUDA compiler
NVCCFLAGS = 	--compiler-options -fno-strict-aliasing \
                --ptxas-options=-v \
                -rdc=true \
                -prec-div=true -ftz=false -prec-sqrt=true -fmad=true

# Flags used by both the CUDA compiler and linker
NVCCFLAG_COMMON = -m64

CONFIG(debug, debug|release) {
        #DEBUG
        NVCCFLAG_COMMON += -g -G
} else {
        #RELEASE
        NVCCFLAG_COMMON += -O2
}

# Prepare intermediat cuda compiler
cudaIntr.input = CUDA_SOURCES
#cudaIntr.output = ${OBJECTS_DIR}${QMAKE_FILE_BASE}_cuda.o
cudaIntr.output = ${QMAKE_FILE_BASE}_cuda.o

cudaIntr.commands = /usr/bin/nvcc $$NVCCFLAG_COMMON -dc $$NVCCFLAGS $$LIBS ${QMAKE_FILE_NAME} -o ${QMAKE_FILE_OUT}

#Set our variable out. These obj files need to be used to create the link obj file and used in our final gcc compilation
cudaIntr.variable_out = CUDA_OBJ
cudaIntr.variable_out += OBJECTS

# Tell Qt that we want add more stuff to the Makefile
QMAKE_EXTRA_UNIX_COMPILERS += cudaIntr

#------------------------- Cuda linker (required for dynamic parallelism)
# Prepare the linking compiler step
cuda.input = CUDA_OBJ
cuda.output = link.o
cuda.CONFIG += combine	# Generate 1 output file

# Tweak arch according to your hw's compute capability
cuda.commands = /usr/bin/nvcc $$NVCCFLAG_COMMON \
                -dlink *_cuda.o -o link.o
cuda.dependency_type = TYPE_C
cuda.depend_command = 	/usr/bin/nvcc -g -G \
                        -M $$NVCCFLAGS ${QMAKE_FILE_NAME}

# Tell Qt that we want add more stuff to the Makefile
QMAKE_EXTRA_UNIX_COMPILERS += cuda
