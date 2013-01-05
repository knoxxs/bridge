import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;


public class Bid{
	char suit;
	char rank;
	public Bid(char a, char b)
	{
		this.suit = a;
		this.rank = b;
	}/*
	public Bid(String json)
	{
		Gson gson = new Gson();
        Bid newBid = gson.fromJson(json, Bid.class); 
	}
*/
	public Bid(Bid newBid)
	{
		this.suit = newBid.suit;
		this.rank = newBid.rank;
	}
	public char getSuit()
	{
		return this.suit;
	}
	public char getRank()
	{
		return this.rank;
	}
	
	public String json_format()
	{
		Gson gson = new Gson();
        String jsonNames = gson.toJson(this);
        return jsonNames;
	}
	/*
	public Bid fromJson(String json)
	{
		Gson gson = new Gson();
		Card newCard = gson.fromJson(json, Card.class); 
	}*/
	public void displayBid()
	{

	}
	public void print()
	{

	}
}