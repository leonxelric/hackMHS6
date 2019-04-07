package client_server_connection_test;

import java.net.*;
import java.io.*;
import java.util.Scanner;

public class Client {
	private final static Scanner fromKeyboard = new Scanner(System.in);
	public static void main(String[] args) throws IOException
	{
		System.out.println("Connecting to server...");
		Socket s = new Socket(args[0], Integer.parseInt(args[1]));
		System.out.println("Connected.");
		
		DataOutputStream out;
		DataInputStream in;
		out = new DataOutputStream(s.getOutputStream());
		in = new DataInputStream(s.getInputStream());

		System.out.println("Message to server?");
		
		byte[] message = new byte[16];
		for(int i = 0; i < message.length; i++)
		{
			message[i] = fromKeyboard.nextByte();
			fromKeyboard.nextLine();
		}
		
		out.write(message);
		System.out.println("Sent to server.");
		
		byte[] info = new byte[16];
		in.readFully(info);
		
		System.out.println("Received from server: ");
		for(byte b : info)
			System.out.println(b);
	}
	
	/*
	 * public static void main(String[] argv){
	 * 	init();
	 * 	for(;;){
	 * 		info = queryInfo();
	 * 		action = decideAction();
	 * 		submitAction(action);
	 * 	}
	 * }
	 * */
}
