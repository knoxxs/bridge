supervisor:supervisor.o accessories.o dispatcher player
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

dispatcher:dispatcher.o access.o psql.o myregex.o
	g++ dispatcher.o access.o psql.o myregex.o -pthread -I /usr/include/postgresql -lpq -o ./bin/dispatcher

dispatcher.o:dispatcher.cpp
	g++ -I /usr/include/postgresql -fpermissive -lpq -c dispatcher.cpp

accessories.o: accessories.c accessories.h
	gcc -c accessories.c

access.o:access.cpp access.h
	g++ -c access.cpp

psql.o:psql.cpp psql.h
	g++ -c psql.cpp -I /usr/include/postgresql -lpq

myregex.o:myregex.cpp myregex.h
	g++ -c myregex.cpp -lm

player:player.o access.o player_helper.o
	g++ player.o access.o player_helper.o -pthread -o ./bin/player

player.o:player.cpp
	g++ -c player.cpp

player_helper.o:player_helper.cpp player_helper.h
	g++ -c -fpermissive player_helper.cpp

cleanAll:cleanO clean~
	
cleanO:
	rm *.o 

clean~:
	rm *~
# sendfdsock:sendfdsock.o accessories.o
# 	gcc -o sendfdsock sendfdsock.o accessories.o

# sendfdsock.o:sendfdsock.c
# 	gcc -c sendfdsock.c
