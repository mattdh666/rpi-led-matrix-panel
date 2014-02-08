RPI_LIB = rgbmatrix

CXXFLAGS = -Wall -O3 -g -I..
LDFLAGS = -L..
LIBS = -lpthread -l$(RPI_LIB)
TARGET = demo

SRCS = RgbMatrixDemo.cpp Thread.cpp
OBJS = $(SRCS:.cpp=.o)


all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(OBJS) $(TARGET)

