class Bid{
	char suit;
	char rank;
	char pos
	public Bid(char a, char b, char c)
	{
		this.suit = a;
		this.rank = b;
		this.pos = c;
	}
	public Card(Card newBid)
	{
		this.suit = newBid.suit;
		this.rank = newBid.rank;
		this.pos = newBid.pos;
	}
	public char getSuit()
	{
		return this.suit;
	}
	public char getRank()
	{
		return this.rank;
	}
	public char getPos()
	{
		return this.Pos;
	}
	public void json_format()
	{

	}
	public void displayBid()
	{

	}
	public void print()
	{

	}
}