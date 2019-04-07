package client_server_connection_test;

import java.net.*;
import java.io.*;

public class Server {
	public static void main(String [] args) throws IOException
	{
		try {
			ServerSocket server = new ServerSocket(Integer.parseInt(args[0]));
			System.out.println("Connecting to client...");
			Socket client = server.accept();
			System.out.println("Connected.");
			
			DataOutputStream out;
			DataInputStream in;
			out = new DataOutputStream(client.getOutputStream());
			in = new DataInputStream(client.getInputStream());
			
			byte[] fromClient = new byte[16];
			in.readFully(fromClient);
			
			byte[] toClient = new byte[16];
			for(int i = 0; i < fromClient.length; i++)
				toClient[i] = (byte) (fromClient[i] - 1);
			
			out.write(toClient);
		}
		catch(SocketException e)
		{
			System.out.println("Connection terminated.");
		}
	}
}
