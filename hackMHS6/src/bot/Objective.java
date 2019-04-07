package bot;

public class Objective {
	private Point location;
	private String action;
	private byte itemId;
	private int num;
	/*
	 * action, additionalInfo, num
	 * find, itemId, num
	 * harvest, itemId, num
	 * sell, itemId, amount, num
	 * refine, goal itemId, num 
	 */
	public Objective(String action, byte itemId, int num)
	{
		this.action = action;
		this.itemId = itemId;
		this.num = num;
	}
	
	public String getAction()
	{
		return action;
	}
	
	public byte getItemId()
	{
		return itemId;
	}
	
	public void addLocation(Point p)
	{
		location = p;
	}
	
	public Point getLocation()
	{
		return location;
	}
	
	public int getNum()
	{
		return num;
	}
}
