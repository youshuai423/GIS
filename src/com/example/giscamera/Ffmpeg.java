package com.example.giscamera;

public class Ffmpeg {
	static public native int open(int devid);
	static public native int init(int width, int height,int numbuf);
	static public native int streamon();
	static public native int dqbuf(byte[] videodata);
	static public native int yuvtorgb(byte[] yuvdata, byte[] rgbdata,int dwidth,int dheight);
	static public native int qbuf(int index);
	static public native int videoinit(byte[] filename);
	static public native int videostart(byte[] yuvdata,int[] time,double[] gps);//,int time[],double gps[]
	static public native int videoclose();
	static public native int streamoff();
	static public native int release();
	static {
		System.loadLibrary("ffmpegutils");
	}
}
