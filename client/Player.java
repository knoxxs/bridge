
import java.util.*;
import java.io.*;
import java.net.*;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

public class Player{
	char position;    // how will i know my pos?
	String plid;
	//char subTeamId;
	//String tid, name, country;
	char declarer;   // will be assigned after getting the result from bid
	char winnerPrevTrick;	// assigned after getting result from winning trick
	String password;
	int score;
	int trickNum = 0;
	int cardsInTrick = 0;
	char suitOfTrick;
	char trump;
	ArrayList<Card> myCards;
	ArrayList<Card> curTrickCard;
	ArrayList<Bid> bids;
	char[] pos = {N,E,S,W};
	Score score;
	Socket kkSocket = null;
    PrintWriter out = null;
    BufferedReader in = null;
	//int fd;
	public Player()
	{
		bids = new ArrayList<Card>();
		myCards = new ArrayList<Bid>();
		curTrickCard = new ArrayList<Card>();
		score = new Score();
		try {
            kkSocket = new Socket("localhost", 4444);
            out = new PrintWriter(kkSocket.getOutputStream(), true);
            in = new BufferedReader(new InputStreamReader(kkSocket.getInputStream()));
        } catch (UnknownHostException e) {
            System.err.println("Don't know about host: taranis.");
            System.exit(1);
        } catch (IOException e) {
            System.err.println("Couldn't get I/O for the connection to: taranis.");
            System.exit(1);
        }
        this.login();
	}
	public void getMsgFromServer()
	{
		String comm;
		in.read(comm,0,5);
		if(comm == "CARDS")
		{
			this.recvAddMyCard();
		}
		else
			if(comm == "CARDO")
			{
				this.recvServerOCard();
			}
			else
				if(comm == "BIDOT")
				{
					this.recvServerBid();
				}
				else
					if(comm == "SCORE")
					{
						this.recvServerScore();
					}
					else
						if(comm == "CARDM")     //assumed when server wants my card, will send me a msg with this thing
						{
							this.chooseCardToBPlay();
						}
						else
							if(comm == "BIDMY")     //same as above
							{
								this.sendMyBid();
							}
	}
	public void recvAddMyCard()
	{
		int i;
		String buf;
		Gson gson = new Gson();
		for(i=0;i<13;i++)
		{

			this.recvAllChar(buf,0,what??);  //TODO: size of each card
			Card newCard = gson.fromJson(json, Card.class); 
			newCard.setOpen(true); 
			myCards.add(newCard);
		}	
	}
	public int sendServerMyCard(Card sendCard)
	{
		String json = sendCard.json_format();
		out.write(json,0,json.length());

			// send
		//if my crd then change countmycards value
	}
	public void recvServerOCard()
	{
		String card;
		this.recvAllChar(card,0,53);
		Gson gson = new Gson();
		Card newCard = gson.fromJson(card, Card.class); 
		this.AddToTrick(newCard);
		//recv in json format and return the card recieved

	}
	public void AddToTrick(Card newCard)
	{
		if(cardsInTrick == 4)
		{	
			trickNum ++;
			curTrickCard.clear();    //starting a new trick
			curTrickCard.add(newCard);
			suitOfTrick = newCard.suit;
			cardsInTrick = 1;
		}
		else
		{
			if(cardsInTrick ==0)
			{
				suitOfTrick = newCard.suit;
			}
			curTrickCard.add(newCard);
			cardsInTrick ++;
		}
	}
	public void showMyCardOnScreen()
	{
		int i=0;
		for(i=0;i<myCards.size();i++)
		{
			myCards.get(i).print();
		}
		//the present cards that i m having
	}
	public Bid recvServerBid()
	{
		Gson gson = new Gson();
        Bid newBid = gson.fromJson(json, Bid.class); 
        bids.add(newBid);
	}
	public int sendMyBid(Bid myBid)
	{
		bids.add(myBid);
		String json = myBid.json_format();
		out.write(json,0,json.length());
		//send
	}
	public void showAllBids()
	{

	}

	public int chooseCardToBPlay()
	{
		System.out.println("Enter your card to be played\n");
		Scanner in = new Scanner(System.in);
		int num = in.nextInt();
		this.playCard(num);

	}
	public int playCard(Card play)
	{
		int i =0;
		for(i=0;i<myCards.size();i++)
		{
			if(myCards.get(i).suit == play.suit && myCards.get(i).rank == play.rank && play.pos == this.position)
			{
				if((myCards.get(i).suit != suitOfTrick && this.hasCardWithSuit(suitOfTrick)) || myCards.get(i).suit == suitOfTrick || myCards.get(i) == trump)
				{
				this.AddToTrick(myCards.remove(i));
				this.sendServerMyCard(play);
				break;
				}
				else
				{
					System.out.println("you cant choose that card,choose again");
					this.chooseCardToBPlay();
				}
			}
		}
			

	}
	public int playCard(int i)     //its only for checking
	{
		if((myCards.get(i).suit != suitOfTrick && this.hasCardWithSuit(suitOfTrick)) || myCards.get(i).suit == suitOfTrick || myCards.get(i) == trump)
		{
			this.AddToTrick(myCards.remove(i));
			this.sendServerMyCard(play);
			break;
		}
		else
		{
			System.out.println("you cant choose that card,choose again");
			this.chooseCardToBPlay();
		}
	}
	public int hasCardWithSuit(char suit)
	{

		int i =0;
		for(i=0;i<myCards.size();i++)
		{
			if(myCards.get(i).suit == suit)
			{
				return 0;
			}
		}
			return 1;
	}
	public int recvServerScore()
	{
		String score;
		String length;
		this.recvAllChar(length,0,2);
		int lenght = Integer.parseInt( length);
		this.recvAllChar(score,0,length);
		Gson gson = new Gson();
		score  = gson.fromJson(card, Score.class); 
	
	}
	public void showScoreOnScreen()
	{
		score.print();
	}
	public int recvServerMsg()
	{

	}
	public void showMsgOnScreen()
	{
		
	}
	public int login()
	{
		System.out.println("Enter your choice \n");
		char choice = stdIn.read();
		System.out.println("Enter your plid \n");
		BufferedReader stdIn = new BufferedReader(new InputStreamReader(System.in));
		this.plid = stdIn.readLine();
		System.out.println("Enter your password \n");
		this.password = stdIn.readLine();
		out.write(choice);
		out.write(plid,0,9);
		out.write(password,0,password.length());
		//send plid and password
		//if valid user, return 1, and initialize other attributes
		//also new Trick
		//if invalid user, return 0
	}
	public void showloginOnScreen()
	{
		
	}
	public void showCurTrickOnScreen(){
		
	}

	public void sendAllChar()
	{

	}

	public void recvAllChar(String buf,int off, int len)
	{
		int left_bytes = len;
		while(left_bytes != 0)
		{
			int read_bytes = in.read(buf,off,left_bytes);
			left_bytes = left_bytes - read_bytes;
			off = off+read_bytes;
		}

	}	
	public void getBidResult()
	{

	}
	public void getTrickResult()
	{

	}
	public void showBidResult()
	{
		// shud get trump, declarer
		//and therefore shud initilise dese attributes
		//and then  
	}
	public void showTrickResult()
	{
		//initialize winner attribute
	}
	public void getGameResult()
	{

	}
	public void showGameResult()
	{

	}
}


// do we need to initialize every details of player.. lyk coutry, team n all
			// but sending my positin os essential
//determing position of the turn ...its not done yet
// sending result and winner of tricks and bidding is left... so a separte command for it as well
// do we need other array of cards fro the dummy player-  as they will be seen in my showMsgOnScreen
//