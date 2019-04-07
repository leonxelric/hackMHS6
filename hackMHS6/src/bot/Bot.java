package bot;

import java.time.Instant;
import java.util.ArrayList;

public class Bot {
	private long userId;
	private int life;
	private int hunger;
	private byte[] inventory;
	private int balance;
	private Point destination;
	private Point playerPos;
	private byte[] view;
	private boolean move;
	private ArrayList<Objective> objectiveQueue; //oQ[0] is current objective
	
	public Bot(byte[] registryInfo)
	{
		view = new byte[25];
		inventory = new byte[16];
		playerPos = new Point(0, 0);
		userId = Converter.byteToLong(registryInfo, 5);
		destination = new Point((int)(playerPos.getX() + Math.random() * 11 - 5), playerPos.getY() + (int)(Math.random() * 11 - 5));
		move = true;
		objectiveQueue = new ArrayList<Objective>();
		objectiveQueue.add(new Objective("find", (byte)TREE, 10));
		life = 1;
	}
	
	public long getId()
	{
		return userId;
	}
	
	public void update(byte[] info, byte[] inventory)
	{
		System.out.println(info[31]);
		for(int i = 0; i < view.length; i++)
			view[i] = info[i + 5];
		life = (int)(info[31]);
		hunger = (int)(info[32]);
		balance = Converter.byteToShort(info, 33);
		for(int i = 0; i < this.inventory.length; i++)
			this.inventory[i] = inventory[i + 5];
	}
	
	public byte[] getRequest() //implement protocol
	{
		Objective currentObjective = objectiveQueue.get(0);
		byte key = currentObjective.getItemId();
		byte tool = (byte)findTool(key);
		if(currentObjective.getAction().equals("find"))
		{
			if(inRange(key) > 0)
			{
				if(indexOf(inventory, tool) > 0)
				{
					objectiveQueue.set(0, new Objective("harvest", currentObjective.getItemId(), currentObjective.getNum() - 1));
					return use(inRange(key), indexOf(inventory, tool));
				}
				else objectiveQueue.add(0, new Objective("refine", tool, currentObjective.getNum() - 1));
			}
			for(int i = 0; i < view.length; i++)
				if(view[i] == key)
				{
					int row = - i / 5 + 2;
					int col = i % 5 - 2;
					destination = new Point(playerPos.getX() + col, playerPos.getY() + row);
				}
		}
		else if(currentObjective.getAction().equals("harvest") && currentObjective.getNum() >= 0)
		{
			objectiveQueue.set(0, new Objective("harvest", currentObjective.getItemId(), currentObjective.getNum() - 1));
			if(currentObjective.getNum() == 0)
				objectiveQueue.add(0, new Objective("refine", tool, 1));
			return use(inRange(key), indexOf(inventory, tool));
		}
		else if(currentObjective.getAction().equals("refine") && currentObjective.getNum() >= 0)
		{
			objectiveQueue.set(0, new Objective("refine", currentObjective.getItemId(), currentObjective.getNum() - 1));
			return refine(tool);
		}
		else if(inventory[indexOf(inventory, (byte)DIRT) + 1] > 5)
			return sell(indexOf(inventory, (byte)DIRT), inventory[indexOf(inventory, (byte)DIRT) + 1]);
		else if(objectiveQueue.size() == 0)
		{
			destination = new Point((int)(Math.random() * 11 - 5), (int)(Math.random() * 11 - 5));
			objectiveQueue.add(new Objective("find", (byte)TREE, 10));
		}
		else if(move)
		{
			if(playerPos.getY() < destination.getY())
			{
				playerPos = new Point(playerPos.getX(), playerPos.getY() + 1);
				return move(1);
			}
			else if(playerPos.getY() > destination.getY())
			{
				playerPos = new Point(playerPos.getX(), playerPos.getY() - 1);
				return move(3);
			}
			else if(playerPos.getX() < destination.getX())
			{
				playerPos = new Point(playerPos.getX() + 1, playerPos.getY());
				return move(2);
			}
			else if(playerPos.getX() > destination.getX())
			{
				playerPos = new Point(playerPos.getX() - 1, playerPos.getY());
				return move(4);
			}
		}
		return null;
	}
	
	private int findTool(int materialId)
	{
		if(materialId == TREE)
			if(indexOf(inventory, (byte)METAL_AXE) != -1)
				return METAL_AXE;
			else
				return WOOD_AXE;
		else if(materialId == DIRT)
			if(indexOf(inventory, (byte)METAL_SHOVEL) != -1)
				return METAL_SHOVEL;
			else
				return WOOD_SHOVEL;
		else if(materialId == STONE || materialId == ORE)
			if(indexOf(inventory, (byte)METAL_PICKAXE) != -1)
				return METAL_PICKAXE;
			else
				return WOOD_PICKAXE;
		else if(materialId == PLANT || materialId == GRASS)
			if(indexOf(inventory, (byte)METAL_SCYTHE) != -1)
				return METAL_SCYTHE;
			else
				return WOOD_SCYTHE;
		return WOOD_AXE;
	}
	
	public byte[] move(int direction)
	{
		byte[] request = new byte[14];
		byte[] id = Converter.longToByte(userId);
		for(int i = 0; i < id.length; i++)
			request[i] = id[i];
		
		long unixTime = Instant.now().getEpochSecond();
		byte[] time = Converter.intToByte((int)unixTime);
		for(int i = id.length; i < 12; i++)
			request[i] = time[i];
		
		request[12] = 1;
		request[13] = (byte)direction;
		return request;
	}
	
	public byte[] sell(int index, int amount)
	{
		byte[] request = new byte[15];
		byte[] id = Converter.longToByte(userId);
		for(int i = 0; i < id.length; i++)
			request[i] = id[i];
		
		long unixTime = Instant.now().getEpochSecond();
		byte[] time = Converter.intToByte((int)unixTime);
		for(int i = id.length; i < 12; i++)
			request[i] = time[i];
		
		request[12] = 6;
		request[13] = (byte)index;
		request[14] = (byte)amount;
		return request;
	}
	
	public byte[] refine(int target)
	{
		byte[] request = new byte[15];
		byte[] id = Converter.longToByte(userId);
		for(int i = 0; i < id.length; i++)
			request[i] = id[i];
		
		long unixTime = Instant.now().getEpochSecond();
		byte[] time = Converter.intToByte((int)unixTime);
		for(int i = id.length; i < 12; i++)
			request[i] = time[i];
		
		request[12] = 10;
		request[13] = (byte)target;
		return request;
	}
	
	public byte[] use(int direction, int itemIndex)
	{
		byte[] request = new byte[15];
		byte[] id = Converter.longToByte(userId);
		for(int i = 0; i < id.length; i++)
			request[i] = id[i];
		
		long unixTime = Instant.now().getEpochSecond();
		byte[] time = Converter.intToByte((int)unixTime);
		for(int i = id.length; i < 12; i++)
			request[i] = time[i];
		
		request[12] = 2;
		request[13] = (byte)direction;
		request[14] = (byte)itemIndex;
		return request;
	}
	
	private int indexOf(byte[] arr, byte key)
	{
		for(int i = 0; i < arr.length; i++)
			if(arr[i] == key)
				return i;
		
		return -1;
	}
	
	private int inRange(byte itemId) //return direction if in range, -1 if not
	{
		if(view[7] == itemId)
			return 1;
		else if(view[13] == itemId)
			return 2;
		else if(view[17] == itemId)
			return 3;
		else if(view[11] == itemId)
			return 4;
		return -1;
	}
	
	public boolean isAlive()
	{
		return life > 0;
	}
	
	public String toString()
	{
		return "life: " + life + "\nbalance: " + balance + "\n";
	}
	
	private static final int EMPTY = 0;
	private static final int PLAYER = 1;
	private static final int DIRT = 2;
	private static final int STONE = 3;
	private static final int ORE = 4;
	private static final int PLANT = 5;
	private static final int TREE = 6;
	private static final int GRASS = 7;
	private static final int WATER = 8;
	private static final int SEED = 9;
	private static final int WOOD_SWORD = 10;
	private static final int WOOD_AXE = 11;
	private static final int WOOD_SHOVEL = 12;
	private static final int WOOD_PICKAXE = 13;
	private static final int WOOD_SCYTHE = 14;
	private static final int STICK = 15;
	private static final int METAL = 16;
	private static final int METAL_SWORD = 17;
	private static final int METAL_AXE = 18;
	private static final int METAL_SHOVEL = 19;
	private static final int METAL_PICKAXE = 20;
	private static final int METAL_SCYTHE = 21;
}