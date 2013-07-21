CXX?=c++

all: screwSSH

screwSSH: screwSSH.cpp Makefile
	${CXX} ${CXXFLAGS} -W -Wall screwSSH.cpp -o screwSSH -lpthread

clean:
	rm -f screwSSH
