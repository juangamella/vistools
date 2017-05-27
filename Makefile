KISS_DIR := ./src
BIN_DIR := ./bin

all: plot plot_stream polar_continuous_show polar_plot polar_show simple_fft gcc_phat

dir:
	mkdir ${BIN_DIR} -p

simple_fft: dir ${KISS_DIR}/kiss_fftr.c ${KISS_DIR}/kiss_fft.c src/simple_fft.c
	gcc -o ./bin/simple_fft ${KISS_DIR}/kiss_fft.c ${KISS_DIR}/kiss_fftr.c src/simple_fft.c -lm -DFIXED_POINT=16

gcc_phat: dir ${KISS_DIR}/kiss_fftr.c ${KISS_DIR}/kiss_fft.c src/gcc_phat.c
	gcc -o ./bin/gcc_phat ${KISS_DIR}/kiss_fft.c ${KISS_DIR}/kiss_fftr.c src/gcc_phat.c -lm

%: dir src/%.c
	gcc -o ${BIN_DIR}/$@ src/$@.c -lX11 -lGL -lGLU -lm

clean:
	rm ${BIN_DIR} -rf

.PHONY: clean all
