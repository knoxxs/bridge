CREATE TABLE Login(
	PLID char(8) PRIMARY KEY,
	password varchar(15) NOT NULL 
);
CREATE TABLE Team(
	TID char(8) PRIMARY KEY,
	Name varchar(30) NOT NULL,
	Country varchar(20) NOT NULL
);
CREATE TABLE Players(
	PID char(8) PRIMARY KEY,
	TID char(8) REFERENCES Team(TID),
	Name varchar(30)
);
CREATE TABLE SubTeam(
	TID char(8) REFERENCES Team(TID),
	STID char(1),
	PID1 char(8) REFERENCES Players(PID),
	PID2 char(8) REFERENCES Players(PID),
	PID3 char(8) REFERENCES Players(PID),
	PID4 char(8) REFERENCES Players(PID),
	CONSTRAINT SubTeam_Key PRIMARY KEY(TID,STID)
);
CREATE TABLE Schedule(
	TID1 char(8) REFERENCES Team(TID),
	TID2 char(8) REFERENCES Team(TID),
	DateTime timestamp,
	Winner char(8) NULL,--TID1, TID2, NULL - not declared, 00000000 - draw
	TID1AScore int,
	TID1BScore int,
	TID2AScore int,
	TID2BScore int,
	EndTimeA time,
	EndTimeB time,
	CONSTRAINT Schedule_Key PRIMARY KEY(TID1,TID2,DateTime)
);
