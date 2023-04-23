SRC_FILES = ./src/*.cpp \
	src/GameState/*.cpp \
	src/Logger/*.cpp \
	src/ECS/*.cpp \

build:
	g++ -O3 -Wall -Wfatal-errors -std=c++20 $(SRC_FILES) -lSDL2 -lSDL2_image -o stabby2d

run:
	./stabby2d

clean:
	rm stabby2d
