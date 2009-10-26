CXXFLAGS=-O3 -Wall
CXX=g++
INSTALL_DIR=/usr/local/bin
INCLUDE_DIR=/usr/local/include

DOAR=src/doar
CMD=src/command
HEADERS=${DOAR}/key_stream.h ${DOAR}/node.h ${DOAR}/searcher.h ${DOAR}/types.h ${DOAR}/static_allocator.h ${DOAR}/dynamic_allocator.h ${DOAR}/double_array.h ${DOAR}/shrink_tail.h ${DOAR}/builder.h ${DOAR}/vector.h

all: bin/doar bin/mkdoar bin/ckdoar

bin/doar: ${CMD}/doar.cc ${HEADERS}
	${CXX} ${CXXFLAGS} -o ${@} ${CMD}/doar.cc

bin/ckdoar: ${CMD}/ckdoar.cc ${HEADERS}
	${CXX} ${CXXFLAGS} -o ${@} ${CMD}/ckdoar.cc

bin/mkdoar: ${CMD}/mkdoar.cc ${HEADERS}
	${CXX} ${CXXFLAGS} -o ${@} ${CMD}/mkdoar.cc

install: bin/doar bin/mkdoar bin/ckdoar
	cp bin/* ${INSTALL_DIR}/
	cp -r ${DOAR} ${INCLUDE_DIR}/ 

clean: 
	rm bin/*