EXT = cpp
CXX=g++
CFLAGS=-Wall -I/urs/local/include
CFLAGS=
LDFLAGS= -lGL -lGLEW -lGLU -lglut  

#SRC=saisieinteractive.cpp courbe.cpp
SRC=$(wildcard *.$(EXT))
OBJ = $(SRC:.$(EXT)=.o)

DEBUBFLAG=-g

BIN=./
EXEC=main
	

all: $(EXEC)
		./$(EXEC)
			
$(EXEC): $(OBJ)
		$(CXX)  $(DEBUBFLAG) -o $(BIN)$@ $^ $(LDFLAGS)

%.o: %.cpp
		$(CXX)  $(DEBUBFLAG) -o $@ -c $< $(CFLAGS)


clean:
		rm -rf *.o

mrproper: clean
		rm -rf $(BIN)$(EXEC)

