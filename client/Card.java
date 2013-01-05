import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;



class Card{
	char suit;
	char rank;
	boolean open=false;
	public Card(char a, char b,char c)
	{
		this.suit = a;
		this.rank = b;
		this.open = c;
	}
	public Card(Card newCard)
	{
		this.suit = newCard.suit;
		this.rank = newCard.rank;
		this.open = newCard.pos;
	}
	public char getSuit()
	{
		return this.suit;
	}
	public char getRank()
	{
		return this.rank;
	}
	public char getOpen()
	{
		return this.open;
	}
	public char setOpen(boolean b)
	{
		this.open = b;
		return this.open;
	}
	public String json_format()
	{
        Gson gson = new Gson();
        String jsonNames = gson.toJson(this);
        return String;
       // System.out.println("jsonNames = " + jsonNames);
	}
	public Card fromJson(String json)
	{

	}
	public void displayCard()
	{

	}
	public void print()
	{
		System.out.println("suit rank open" + this.suit +" " + this.rank + " " +  this.open);
	}
	public void exchangeCards(Card otherCard)
	{
		char s = this.suit;
		char r = this.rank;
		char p = this.pos;
		this.suit = otherCard.suit;
		this.rank = otherCard.rank;
		this.pos = otherCard.pos;
		otherCard.suit = s;
		otherCard.rank = r;
		otherCard.pos = p;
	}

}