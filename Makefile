CC = cc

CFLAGS = -O0 -g -pthread -Wall -Wextra -pedantic -std=c99

LIBS = -ldiscord -lcurl

SRC = bot.c
OBJ = $(SRC:.c=.o)

TARGET = bot

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
