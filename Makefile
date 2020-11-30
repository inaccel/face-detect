CXXFLAGS = -O3 -std=c++11 -Wall

ifeq (${TARGET}, sw)
ifeq (${SAVE}, yes)
CXX_SRCS = sw/face_detect_save.cpp sw/rectangles.cpp sw/haar.cpp sw/image.cpp sw/stdio-wrapper.cpp
else
CXX_SRCS = sw/face_detect_view.cpp sw/rectangles.cpp sw/haar.cpp sw/image.cpp sw/stdio-wrapper.cpp
endif
else
ifeq (${TARGET}, hw)
ifeq (${SAVE}, yes)
CXX_SRCS = hw/face_detect_save.cpp hw/rectangles.cpp hw/utils.cpp
else
CXX_SRCS = hw/face_detect_view.cpp hw/rectangles.cpp hw/utils.cpp
endif
else
$(error TARGET must either be defined as 'hw' or 'sw')
endif
endif

OBJECTS = $(CXX_SRCS:.cpp=.o)

LDLIBS = -lpthread -lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_highgui -lcoral-api

all: face_detect_${TARGET}

face_detect_${TARGET}: ${OBJECTS}
	$(CXX) $(^) $(LDLIBS) -o $(@)

clean:
	rm -f ${OBJECTS} face_detect_${TARGET}
