all:supervisor dispatcher player game cleanO

supervisor:supervisor.o accessories.o
	gcc -o ./bin/supervisor  supervisor.o accessories.o

supervisor.o:supervisor.c accessories.h
	gcc -c supervisor.c

compileAll:sendfdsock recvfdsock

sendfdsock.o:sendfdsock.c accessories.h
	gcc -c sendfdsock.c

sendfdsock:sendfdsock.o accessories.o
	gcc -o ./bin/sendfdsock sendfdsock.o accessories.o

recvfdsock.o:recvfdsock.c accessories.h
	gcc -c recvfdsock.c

recvfdsock:recvfdsock.o accessories.o
	gcc -o ./bin/recvfdsock recvfdsock.o accessories.o

dispatcher:dispatcher.o access.o psql.o myregex.o helper.o
	g++ dispatcher.o access.o psql.o myregex.o helper.o -pthread -I /usr/include/postgresql -lpq -o ./bin/dispatcher

dispatcher.o:dispatcher.cpp
	g++ -I /usr/include/postgresql -fpermissive -lpq -c dispatcher.cpp

accessories.o: accessories.c accessories.h
	gcc -c accessories.c

access.o:access.cpp access.h
	g++ -std=c++0x -c access.cpp

psql.o:psql.cpp psql.h
	g++ -c psql.cpp -I /usr/include/postgresql -lpq

#myregex:myregex.o access.o
#	g++ myregex.o access.o -o ./bin/myregex

myregex.o:myregex.cpp myregex.h
	g++ -c myregex.cpp -lm

helper.o:helper.h helper.cpp
	g++ -I /usr/include/postgresql -lpq -fpermissive -c helper.cpp

player:player.o access.o psql.o helper.o
	g++ player.o access.o psql.o helper.o -pthread -I /usr/include/postgresql -lpq -o ./bin/player

player.o:player.cpp
	g++ -I /usr/include/postgresql -lpq -fpermissive -c player.cpp

game:game.o access.o psql.o helper.o msgQ.o
	g++ game.o access.o psql.o helper.o msgQ.o -pthread -I /usr/include/postgresql -lpq -o ./bin/game

game.o:game.cpp game.h
	g++ -std=c++0x -I /usr/include/postgresql -lpq -fpermissive -c game.cpp

msgQ.o:msgQ.cpp msgQ.h
	g++ -c msgQ.cpp
#psql_player.o:psql_player.cpp psql_player.h
#	g++ -c psql_player.cpp -I /usr/include/postgresql -lpq

cleanAll:cleanO clean~
	
cleanO:
	rm *.o 

clean~:
	rm *~
# sendfdsock:sendfdsock.o accessories.o
# 	gcc -o sendfdsock sendfdsock.o accessories.o

# sendfdsock.o:sendfdsock.c
# 	gcc -c sendfdsock.c
