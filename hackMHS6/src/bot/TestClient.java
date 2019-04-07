package bot;

import java.net.*;
import java.time.Instant;
import java.util.Arrays;
import java.io.*;

public class TestClient {
	private static DataInputStream in;
	private static DataOutputStream out;
	
	public static void main(String[] args) throws UnknownHostException, IOException
	{
		Socket socket = new Socket("96.225.97.220", 12345);
		in = new DataInputStream(socket.getInputStream());
		out = new DataOutputStream(socket.getOutputStream());
		
		byte[] registerResponse = register();
		out.write(info(registerResponse));
		byte[] response = new byte[35];
		int n = in.read(response);
		System.out.println(Arrays.toString(response));
		System.out.println(n);
	}
	
	public static byte[] info(byte[] registryInfo) throws IOException
	{
		byte request[] = new byte[13];
		for(int i = 0; i < 8; i++)
			request[i] = registryInfo[i + 5];
		long unixTime = Instant.now().getEpochSecond();
		byte[] time = Converter.intToByte((int)unixTime);
		for(int i = 8; i < 12; i++)
			request[i] = time[i - 8];
		
		request[12] = 3;
		return request;
	}
	
	public static byte[] register() throws IOException
	{
		byte[] request = new byte[13];
		out.write(request);
		byte[] result = new byte[15];
		in.readFully(result);

		return result;
	}
}
