CXX?=c++

all: screwSSH

screwSSH: screwSSH.cpp Makefile
	${CXX} ${CXXFLAGS} -W -Wall -lpthread screwSSH.cpp -o screwSSH

clean:
	rm -f screwSSH
