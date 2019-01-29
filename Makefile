NAME =	libosureplayparser.a

FILE =	src/osu_replay_parser.c		\
		src/uncompress_lzma.c		\
		lzma/src/common_internal.c	\
		lzma/src/compress.c			\
		lzma/src/decompress.c		\
		lzma/src/lzip_header.c		\
		lzma/src/lzma_header.c		\
		lzma/src/pavlov/7zBuf.c		\
		lzma/src/pavlov/7zBuf2.c	\
		lzma/src/pavlov/7zCrc.c		\
		lzma/src/pavlov/7zFile.c	\
		lzma/src/pavlov/7zStream.c	\
		lzma/src/pavlov/Alloc.c		\
		lzma/src/pavlov/Bcj2.c		\
		lzma/src/pavlov/Bra.c		\
		lzma/src/pavlov/Bra86.c		\
		lzma/src/pavlov/BraIA64.c	\
		lzma/src/pavlov/LzFind.c	\
		lzma/src/pavlov/LzmaDec.c	\
		lzma/src/pavlov/LzmaEnc.c	\
		lzma/src/pavlov/LzmaLib.c	\

SRC =	$(FILE:%.c=%.o)

OBJ =	$(SRC:.c=.o)

INC =	-Iinclude			\
		-Ilzma/src			\
		-Ilzma/src/pavlov	\
		-Ilzma/src/easylzma	\

CFLAGS=	$(INC)	\
	-W			\
	-Wall		\
	-Wextra		\

CC =	gcc

all:	$(NAME)

$(NAME):$(OBJ)
	$(AR) rc $(NAME) $(OBJ)

clean:
	$(RM) $(OBJ)

fclean:	clean
	$(RM) $(NAME)

re:	fclean all

dbg:	CFLAGS += -g -O0
dbg:	re
