CC		=	gcc
CFLAGS	=	-DDEBUG -std=c11 -g3
TARGET	=	tb
SRCS	=	tb.c
OBJS	=	$(SRCS:.cpp=.o)
INCDIR	=	-I./
LIBDIR	=
LIBS	=

$(TARGET):	$(OBJS)
	$(CC) -o $(TARGET) $(SRCS) $(CFLAGS) $(LIBDIR) $(LIBS)

all: clean $(OBJS) $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) *.d
