CXXFLAGS=-Wall -O3 -g
LDFLAGS=-lpthread
OBJECTS=RgbMatrixDemo.o GpioProxy.o RgbMatrix.o Thread.o
TARGET=demo

all: $(TARGET)

RgbMatrix.o: RgbMatrix.cpp RgbMatrix.h
RgbMatrixDemo.o: RgbMatrix.h

demo: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJECTS) $(TARGET)

