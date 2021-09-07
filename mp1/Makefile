CC=/usr/bin/gcc
CC_OPTS=-g3
CC_LIBS=
CC_DEFINES=
CC_INCLUDES=
CC_ARGS=${CC_OPTS} ${CC_LIBS} ${CC_DEFINES} ${CC_INCLUDES}

# clean is not a file
.PHONY=clean

#target "all" depends on all others
all: http_client http_server

# client C depends on source file client.c, if that changes, make client will 
# rebuild the binary
http_client: http_client.c
	@${CC} ${CC_ARGS} -o http_client http_client.c

http_server: http_server.c
	@${CC} ${CC_ARGS} -o http_server http_server.c

