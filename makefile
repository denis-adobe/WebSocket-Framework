CC=g++
OBJECTS=Base64Encoder.o sha1.o WebSocket.o main.o
PROJECT=WebSocket
EXECUTABLE=WebSocket

all: $(EXECUTABLE)

$(PROJECT): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(PROJECT)

main.o: main.cpp
	$(CC) -c main.cpp

Base64Encoder.o: Base64Encoder.cpp Base64Encoder.h
	$(CC) -c Base64Encoder.cpp

sha1.o: sha1.cpp sha1.h
	$(CC) -c sha1.cpp

WebSocket.o: WebSocket.cpp WebSocket.h
	$(CC) -c WebSocket.cpp

clean:
	rm -rf $(OBJECTS)
