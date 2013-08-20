RPI_LIB = rgbmatrix

CXXFLAGS = -Wall -O3 -g
INCLUDES = -I..
LDFLAGS = -L..
LIBS = -lpthread -l$(RPI_LIB)

SRCS = RgbMatrixDemo.cpp Thread.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = demo

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(OBJS) $(TARGET)

