DROP TABLE SubTeam;
DROP TABLE Login;
DROP TABLE Players;
DROP TABLE Schedule;
DROP TABLE Team;
CREATE TABLE Team(
	TID char(8) PRIMARY KEY,
	Name varchar(30) NOT NULL,
	Country varchar(20) NOT NULL
);
CREATE TABLE Players(
	PID char(8) PRIMARY KEY,
--	TID char(8) REFERENCES Team(TID),
	Name varchar(30)
);
CREATE TABLE Login(
	PLID char(8) REFERENCES Players(PID) PRIMARY KEY,
	password varchar(15) NOT NULL 
);
CREATE TABLE SubTeam(
	TID char(8) REFERENCES Team(TID),
	STID char(1),
	PID char(8) REFERENCES Players(PID),
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


INSERT INTO Team VALUES('teamID01', 'Team Name', 'Country');
INSERT INTO Team VALUES('teamID02', 'Team Name', 'Country');
INSERT INTO Players VALUES('11111000', 'Player Name');
INSERT INTO Login VALUES('11111000', 'abcd');
INSERT INTO SUBTEAM VALUES('teamID01','A', '11111000');
INSERT INTO Schedule VALUES('teamID01','teamID02', '2012-01-15 04:05:06');

CREATE OR REPLACE FUNCTION getPlayerSchedule(plid char(8)) RETURNS timestamp As
$$
DECLARE
	t timestamp;
BEGIN
	SELECT min(datetime) INTO t FROM SCHEDULE JOIN (SELECT TID FROM SUBTEAM WHERE plid = pid) as Q ON tid = Q.tid;
	return t;
END
$$ LANGUAGE plpgsql;

--Select * From getPlayerSchedule('11111000');

CREATE OR REPLACE FUNCTION getPlayerInfo(plid char(8), OUT tid char(8), OUT name varchar(30)) As
$$
DECLARE
BEGIN
	SELECT PLAYERS.name INTO name FROM PLAYERS WHERE pid = plid;
	SELECT SubTeam.tid INTO tid FROM SubTeam NATURAL JOIN PLAYERS WHERE pid = plid;
	return;
END
$$ LANGUAGE plpgsql;

--Select * From getPlayerInfo('11111000');
