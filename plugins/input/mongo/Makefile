
CXX = g++ -fPIC -O3 -DNDEBUG -Wall

CXXFLAGS = $(shell mapnik-config --cflags)

LIB = -lmongoclient

LIBS = $(LIB) $(shell mapnik-config --libs --dep-libs)

SRC = $(wildcard *.cpp)

OBJ = $(SRC:.cpp=.o)

BIN = mongo.input

all : $(SRC) $(BIN)

$(BIN) : $(OBJ)
	$(CXX) -shared $(OBJ) $(LIBS) -o $@

.cpp.o :
	$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY : clean

clean:
	rm -f $(OBJ)
	rm -f $(BIN)

deploy : all
	cp $(BIN) $(shell mapnik-config --input-plugins)

install: clean all deploy
