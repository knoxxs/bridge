	
import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;



public class Card{
	char suit;
	char rank;
	boolean open=false;
	public Card(char a, char b,boolean c)
	{
		this.suit = a;
		this.rank = b;
		this.open = c;
	}
	public Card(Card newCard)
	{
		this.suit = newCard.suit;
		this.rank = newCard.rank;
		this.open = newCard.open;
	}
	public char getSuit()
	{
		return this.suit;
	}
	public char getRank()
	{
		return this.rank;
	}
	public boolean getOpen()
	{
		return this.open;
	}
	public void setOpen(boolean b)
	{
		this.open = b;
	}
	public String json_format()
	{
        Gson gson = new Gson();
        String jsonNames = gson.toJson(this);
        return jsonNames;
       // System.out.println("jsonNames = " + jsonNames);
	}
	
	public void displayCard()
	{

	}
	public void print()
	{
		System.out.println("suit rank open" + this.suit +" " + this.rank + " " +  this.open);
	}/*
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
*/
}