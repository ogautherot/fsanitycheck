#
#
#

CC=gcc
CXX=g++
DIR:=$(PWD)


TARGET=fsanitycheck
OBJS=obj/main.o obj/initfile.o obj/applib.o obj/dbconn.o 

CFLAGS=-c -g -Og -I$(DIR)
LDFLAGS=-Og -g 


$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) -lpq -lssl -lcrypto

obj/%.o: src/%.c 
	$(CC) $(CFLAGS) -std=c17 $< -o $@

obj/%.o: src/%.cpp 
	$(CXX) $(CFLAGS) -std=c++17 $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)


