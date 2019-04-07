package bot;

public class Converter {
	public static short byteToShort(byte[] b, int start){
		short result = 0;
		for(int i = start; i < start + 2; i++) {
			result |= ((short) b[i]) << (i * 8);
		}
		return result;
	}
	
	public static int byteToInt(byte[] b, int start){
		int result = 0;
		for(int i = start; i < start + 4; i++) {
			result |= ((int) b[i]) << (i * 8);
		}
		return result;
	}
	
	public static long byteToLong(byte[] b, int start){
		long result = 0;
		for(int i = start; i < start + 8; i++) {
			result |= ((long) b[i]) << (i * 8);
		}
		return result;
	}
	
	public static byte[] shortToByte(short s){
		byte[] result = new byte[2];
		for(int i = 0; i < 2; i++) {
			result[i] = (byte)(s >> (i * 8));
		}
		return result;
	}
	
	public static byte[] intToByte(int i){
		byte[] result = new byte[4];
		for(int index = 0; index < 4; index++) {
			result[index] = (byte)(i >> (index * 8));
		}
		return result;
	}
	
	public static byte[] longToByte(long l){
		byte[] result = new byte[8];
		for(int i = 0; i < 8; i++) {
			result[i] = (byte)(l >> (i * 8));
		}
		return result;
	}
}
