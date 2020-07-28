CC = g++
PROGRAM_NAME = rtweekend
SOURCES = $(wildcard *.cpp)
OBJS = $(SOURCES:.cpp=.o)
DEPS = 
CPPFLAGS = -Wall -Wextra -fopenmp -Wno-unused-parameter -lSDL2

.PHONY: all clean distclean

all: $(PROGRAM_NAME)

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS)

$(PROGRAM_NAME): $(OBJS)
	$(CC) $^ $(CPPFLAGS) -o $(PROGRAM_NAME)

#$(SOURCES:.c=.o): $(HEADERS)

run: $(PROGRAM_NAME)
	./$(PROGRAM_NAME)

valgrind: $(PROGRAM_NAME)
	valgrind --suppressions=valgrind-suppressions.supp --leak-check=yes ./$(PROGRAM_NAME)

clean:
	rm -f $(PROGRAM_NAME)
	rm -f *.o

distclean: clean
