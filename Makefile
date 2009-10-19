OPT=-O3 -Wall
DOAR=src/doar
CMD=src/command
HEADERS=${DOAR}/key_stream.h ${DOAR}/node.h ${DOAR}/searcher.h ${DOAR}/types.h ${DOAR}/static_allocator.h ${DOAR}/dynamic_allocator.h ${DOAR}/double_array.h ${DOAR}/shrink_tail.h ${DOAR}/builder.h ${DOAR}/vector.h

all: bin/doar bin/mkdoar bin/doar_test bin/mkddar

bin/doar: ${CMD}/doar.cc ${HEADERS}
	g++ ${OPT} -o${@} ${CMD}/doar.cc

bin/doar_test: ${CMD}/doar_test.cc ${HEADERS}
	g++ ${OPT} -o${@} ${CMD}/doar_test.cc

bin/mkdoar: ${CMD}/mkdoar.cc ${HEADERS}
	g++ ${OPT} -o${@} ${CMD}/mkdoar.cc

bin/mkddar: ${CMD}/mkddar.cc ${HEADERS}
	g++ ${OPT} -o${@} ${CMD}/mkddar.cc
