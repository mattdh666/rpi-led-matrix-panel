# Create a static library for controlling an RGB Matrix.

CXXFLAGS = -fPIC -Wall -O3 -g
TARGET_LIB = librgbmatrix.a

SRCS = GpioProxy.cpp RgbMatrix.cpp
OBJS = $(SRCS:.cpp=.o)


all: $(TARGET_LIB)

$(TARGET_LIB): $(OBJS)
	ar -rs $@ $^

$(SRCS:.cpp=.d):%.d:%.cpp
	$(CXX) $(CXXFLAGS) -MM $< >$@@

clean:
	rm -f $(OBJS) $(TARGET_LIB) $(SRCS:.cpp=.d)

