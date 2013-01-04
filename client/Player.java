import java.util.*;

class Player{
	char position;
	String plid;
	char subTeamId;
	String tid, name, country;
	String password;
	int countMyCards =0;
	Card myCards[13];
	ArrayList Bids;
	//int fd;
	public Player()
	{
		Bids = new ArrayList();
	}
	public void addCard(Card newCard)
	{
		countMyCards ++;
		myCards[i] = new Card(newCard);
	}
	public int sendServerCard(Card sendCard)
	{
			//convert into json and then send
	}
	public Card recvServerCard()
	{
		//recv in json format and return the card recieved
	}
	public void showMyCardOnScreen()
	{
		//the present cards that i m having
	}
	public Bid recvServerBid()
	{

	}
	pubic int sendMyBid(Bid MyBid)
	{

	}
	public void showAllBids()
	{

	}
	public int hasCardWithSuit(char suit)
	{

	}
	public int recvServerScore()
	{

	}
	public void showScoreOnScreen()
	{
		
	}
	public int recvServerMsg()
	{

	}
	public void showMsgOnScreen()
	{
		
	}
	public int login()
	{

	}
	public void showloginOnScreen()
	{
		
	}
	public void showCurTrickOnScreen(){
		
	}

}