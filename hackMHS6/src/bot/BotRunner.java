package bot;

import java.net.*;
import java.time.Instant;
import java.util.Arrays;
import java.util.concurrent.TimeUnit;
import java.io.*;

public class BotRunner {
	private static Socket socket;
	private static String serverAddress;
	private static int port;
	private static Bot player;
	private static DataInputStream in;
	private static DataOutputStream out;
	
	public static void main(String[] args) throws IOException, InterruptedException
	{ 
		init(args);
		while(player.isAlive())
		{
			out.write(info());
			
			byte[] info = new byte[35];
			
			in.read(info);
			System.out.println(Arrays.toString(info));
			/////////////////////////////////
			out.write(queryInventory());
			byte[] inventory = new byte[21];
			in.read(inventory);
			
			player.update(info, inventory);
			
			if(player.getRequest() != null)
				out.write(player.getRequest()); //performs action
			System.out.println(player);
			
			socket.close();
			
			
			
			socket = new Socket(serverAddress, port);
			ioSetup(socket);
			
			
		}
	}
	
	private static void init(String[] args) throws IOException
	{
		serverAddress = args[0];
		port = -1;
		try {
			port = Integer.parseInt(args[1]);
		}
		catch(NumberFormatException e) {
			System.err.println("Port must be an int.");
			return;
		}
		
		System.out.println("Connecting to server...");
		socket = new Socket(serverAddress, port);
		System.out.println("Connection successful!");
		ioSetup(socket);
		
		player = new Bot(register());
	}
	
	private static void ioSetup(Socket s) throws IOException
	{
		in = new DataInputStream(s.getInputStream());
		out = new DataOutputStream(s.getOutputStream());
	}
	
	private static byte[] register() throws IOException
	{
		out.write(new byte[13]); //empty array, a[12] = 0 means register, rest is irrelevant
		
		byte[] response = new byte[13];
		in.read(response);
		
		return response;
	}
	
	private static byte[] info() throws IOException 
	{
		byte[] request = new byte[14];
		byte[] id = Converter.longToByte(player.getId());
		for(int i = 0; i < id.length; i++)
			request[i] = id[i];
		long unixTime = Instant.now().getEpochSecond();
//		System.out.println("id: " +  player.getId());
//		System.out.println(unixTime);
		byte[] time = Converter.intToByte((int)unixTime);
		for(int i = id.length; i < 12; i++)
			request[i] = time[i - id.length];
		
		request[12] = 3;
		
		System.out.println("request: " + Arrays.toString(request));
		
		return request;
	}
	
	private static byte[] queryInventory() throws IOException //todo
	{

		byte[] request = new byte[14];
		byte[] id = Converter.longToByte(player.getId());
		for(int i = 0; i < id.length; i++)
			request[i] = id[i];
		long unixTime = Instant.now().getEpochSecond();
		byte[] time = Converter.intToByte((int)unixTime);
		for(int i = id.length; i < 12; i++)
			request[i] = time[i - id.length];
		
		request[12] = 8;
		return request;
	}
}
