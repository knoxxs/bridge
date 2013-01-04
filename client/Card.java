
class Card{
	char suit;
	char rank;
	char pos;
	public Card(char a, char b,char c)
	{
		this.suit = a;
		this.rank = b;
		this.pos = c;
	}
	public Card(Card newCard)
	{
		this.suit = newCard.suit;
		this.rank = newCard.rank;
		this.pos = newCard.pos;
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
	public void displayCard()
	{

	}
	public void print()
	{

	}

}