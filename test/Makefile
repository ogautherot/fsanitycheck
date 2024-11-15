
TARGET=upload
OBJS=upload.o

.PHONY: all clean test

#all: ${TARGET}
all: upload patch_len sign_files
upload: upload.cpp
	${CXX} -g -o upload upload.cpp -lpq

patch_len: patch_len.cpp
	${CXX} -g -o patch_len patch_len.cpp -lpq

sign_files: sign_files.cpp
	${CXX} -g -o sign_files sign_files.cpp -lcrypto -lpq 

${TARGET}: ${OBJS}
	${CXX} -g ${OBJS} -o ${TARGET} -lpq

.cpp.o:
	${CXX} -c -g -Wall -Wextra -Og $< -o $@

clean:
	rm -f ${OBJS} ${TARGET}

test:
	./${TARGET} zssd.files

