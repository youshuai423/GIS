package com.example.giscamera;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Calendar;
import java.util.List;

import android.app.Activity;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.StatFs;
import android.os.SystemClock;
import android.provider.Settings;
import android.text.format.Time;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

public class Webcam extends Activity{
	
	public static final int UPDATE_VIDEO = 1;  // msg-屏幕刷新
	public static final int SHORT_OF_SIZE = 2;  // msg-容量不足
	public static final int GPS_DIALOG = 3;  // msg-查找GPS对话框
	public static final int GPS_CANCEL_DIALOG = 4;  // msg-取消查找GPS对话框
	
	public static int save_interval = 0;  // 保存间隔（min）
	public static boolean savedir = false; // true-内置，false-外置 
	public static final int save_width = 800;  // 保存像素
	public static final int save_height = 600;
	public static final int screen_width = 480;  // 输出屏幕像素
	public static final int screen_height = 272;
	private int numbuf = 4;
	private int devid = 4;  // 类似com口
	private int ret = 0;
	
	private byte[] pixel_save;  // 保存像素
	private byte[] pixel_screen;  // 屏幕像素
	
	/*压缩屏幕显示*/
	private int screennum=0;
	private int screennums = 1;
	
	/*加密信息*/
	private long utctime = 0; // GPS获取UTC时间
	private Calendar calendar;  // 将UTC时间分解成具体日期和时间
	public static int[] Date; // GPS：Year, Month, Day, Hour, Minute, Second;
	public static double[] pGps; //GPS： longtitude, latitude, altitude;
	
	private boolean screen_pressed = true;  // 判断屏幕按下开始或结束
	private boolean en_video = false;  // 视频开始标志
	private boolean record_stop = false;  // 视频停止标志
	private boolean locationChanged = false;  // 定位成功标志
	
	private String SDpath = null;
	private String	videodir;  // 视频输出路径
	public static String txtdir;  // 文件输出路径
	private String TAG = "FfmpegUtils";
	
	private View button_screen;  // 全屏幕按钮
	private Bitmap bitmap;
	private ByteBuffer Imagbuf;
	private ImageView Video_Image;  // 视频
	private Time mtime;
	private LocationManager lm;
	private String provider;  // 位置判断方法：GPS 或 network
	public static Location sharelocation;
	private TextView text_view_latitude; 
	private TextView text_view_longtitude;
	private TextView text_view_altitude;
	private TextView text_view_speed;
	public static TextView text_view_time;  // 录制时间
	private ProgressDialog progressDialog;  // GPS搜索对话框
	private AlarmReceiver2 alarmreceiver2 = new AlarmReceiver2();  // GPS搜索失败广播接受器
	private IntentFilter intentFilter2 = new IntentFilter();  //
	private AlarmReceiver3 alarmreceiver3 = new AlarmReceiver3();  // 设定时间结束录制-广播接收器
	private IntentFilter intentFilter3 = new IntentFilter();  // 
	private AlarmReceiver4 alarmreceiver4 = new AlarmReceiver4();  // 设定时间开始录制-广播接收器
	private IntentFilter intentFilter4 = new IntentFilter();  // 
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		super.requestWindowFeature(Window.FEATURE_NO_TITLE);
		super.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);  // 全屏
		setContentView(R.layout.webcam);  
		
		pixel_save = new byte[save_width * save_height * 2];
		pixel_screen = new byte[screen_width * screen_height * 2];		
		button_screen = (View)findViewById(R.id.view1);
		Video_Image = (ImageView)findViewById(R.id.mimg);
		bitmap = Bitmap.createBitmap(screen_width, screen_height, Config.RGB_565);
		Imagbuf = ByteBuffer.wrap(pixel_screen); 
		mtime = new Time();
		text_view_latitude = (TextView) findViewById(R.id.textView1);
		text_view_longtitude = (TextView) findViewById(R.id.textView2);
		text_view_altitude = (TextView) findViewById(R.id.textView3);
		text_view_speed = (TextView) findViewById(R.id.textView4);
		text_view_time = (TextView) findViewById(R.id.textView5);
		progressDialog = new ProgressDialog(this);		

		calendar = Calendar.getInstance();
		Date = new int[6];
		pGps = new double[3];
		
		intentFilter2.addAction("com.example.giscamera.ALARM2");  // 接受自定义广播ALARM2
		intentFilter3.addAction("com.example.giscamera.ALARM3");  // 接受自定义广播ALARM3
		intentFilter4.addAction("com.example.giscamera.ALARM4");  // 接受自定义广播ALARM4
		
		VideoInit();  // 初始化视频
		LocationInit();	  // 初始化GPS

	}
	
	private void VideoInit() {
		/*------------------------------------------------------*
		 * -----------------Video operation------------------*
		 *------------------------------------------------------*/
		Log.e("lifecycle","VideoInit");
		ret = Ffmpeg.open(devid); // 打开摄像头
		if(ret < 0) {
			Toast.makeText(Webcam.this, "Open device failed", Toast.LENGTH_LONG).show();
			onDestroy();
		}
		ret = Ffmpeg.init(save_width, save_height, numbuf); // 初始化
		if(ret < 0) {
			Toast.makeText(Webcam.this, "Init device failed", Toast.LENGTH_LONG).show();
			onDestroy();
		}
		ret = Ffmpeg.streamon();
		if(ret < 0) {
			Toast.makeText(Webcam.this, "Stream on failed", Toast.LENGTH_LONG).show();
			onDestroy();
		}
		button_screen.setOnClickListener(videolistener);
		new VideoThread().start();  // 屏幕显示线程
	}
	
	private void LocationInit() {
		/*------------------------------------------------------*
		 * ---------------Location operation-----------------*
		 *------------------------------------------------------*/
		Log.e("lifecycle","LocationInit");
		lm = (LocationManager) getSystemService(Context.LOCATION_SERVICE);  
		// 判断GPS是否正常启动
		if (!lm.isProviderEnabled(LocationManager.GPS_PROVIDER))
		{
			Toast.makeText(this, "请开启GPS导航...", Toast.LENGTH_SHORT).show();
			// 返回开启GPS导航设置界面
			Intent intent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
			//startActivityForResult(intent, 0);
			startActivity(intent);
			return;
		}
		
		List<String> providerList = lm.getProviders(true);  // 获取位置提供器
		if (providerList.contains(LocationManager.GPS_PROVIDER)) {
			provider = LocationManager.GPS_PROVIDER;
			//Toast.makeText(this, "GPS_PROVIDER to use", Toast.LENGTH_SHORT).show();
		}
		else if (providerList.contains(LocationManager.NETWORK_PROVIDER)) {
			provider = LocationManager.NETWORK_PROVIDER;
			Toast.makeText(this, "NETWORK_PROVIDER", Toast.LENGTH_SHORT).show();
		}
		else {
			Toast.makeText(this, "No location provider to use", Toast.LENGTH_SHORT).show();
			return;
		}
		
		lm.requestLocationUpdates(provider, 100, 1, myLocationListener);   // 1000ms或1m更新一次
		
	}

  	/*------------------------------------------------------*
  	 * ---------------检测GPS----------------*
  	 *------------------------------------------------------*/
     public class GPSwatch extends Thread {

    	 @Override
    	 public void run() {
			    Message message1 = new Message();
			    Message message2 = new Message();
    		 if(!locationChanged) {
    		    // 提示正在搜索GPS
    			 Log.e("lifecycle","searchGPS");
				message1.what = GPS_DIALOG;
				handler.sendMessage(message1);
    		 }
				AlarmManager alarmmanager = (AlarmManager) getSystemService(ALARM_SERVICE);
		        int triggertime = 5 * 60 * 1000; // 5min
		        long triggerAtTime = SystemClock.elapsedRealtime() + triggertime;  // 唤醒时刻 = 系统时间 + triggertime
		        Intent i = new Intent("com.example.giscamera.ALARM2");  // 自定义动作ALARM2
		        PendingIntent pi = PendingIntent.getBroadcast(Webcam.this, 0, i, 0);  // 注册广播
		        alarmmanager.set(AlarmManager.ELAPSED_REALTIME_WAKEUP, triggerAtTime, pi);  // 启动AlarmManager
		        
				while(!locationChanged) {
					// 未检测到GPS；
				}
				
				alarmmanager.cancel(pi); 
				
				videodir = SDpath + "/DCMI/gm"+ Date[0]%100 + Date[1] + Date[2] + "_"
						+ Date[3] + Date[4] + Date[5] + ".avi";
				
				txtdir = SDpath + "/DCMI/gm"+ Date[0]%100 + Date[1] + Date[2] + "_"
						+ Date[3] + Date[4] + Date[5] + ".txt";
				
				byte s1[];
				s1=videodir.getBytes();
				
				ret = Ffmpeg.videoinit(s1);
				
				
				if(ret < 0) {
					Toast.makeText(Webcam.this, "++video initfailed", Toast.LENGTH_LONG).show();
					//onDestroy();
				}
				Log.e("lifecycle","videoinit");
				// 新建txt文本
				File file = new File(txtdir);
				try {
					file.createNewFile();
				} catch (IOException e) {
					e.printStackTrace();
				}
				Log.e("lifecycle","newFile");
				if(!screen_pressed) {
					new SvideoThread().start();  // 录制视频线程
					
					// 开始定时后台服务
					Intent startintent = new Intent(Webcam.this, EverySecService.class);
					startintent.putExtra("alarm", 2);
					startService(startintent);
					
					new FreeSizeThread().start();  // 剩余空间检测线程
					
					message2.what = GPS_CANCEL_DIALOG;
					handler.sendMessage(message2);
					
					// 在设定的时间后结束视频，重新开始
					if (save_interval > 0) {
						AlarmManager alarmmanager3 = (AlarmManager) getSystemService(ALARM_SERVICE);
						int triggertime3 = save_interval * 60 * 1000; // 20min
						long triggerAtTime3 = SystemClock.elapsedRealtime() + triggertime3;  // 唤醒时刻 = 系统时间 + triggertime
						Intent alarmintent3 = new Intent("com.example.giscamera.ALARM3");  // 自定义动作ALARM2
						PendingIntent pi3 = PendingIntent.getBroadcast(Webcam.this, 0, alarmintent3, 0);  // 注册广播
						alarmmanager3.set(AlarmManager.ELAPSED_REALTIME_WAKEUP, triggerAtTime3, pi3);  // 启动AlarmManager
					}
				}
    	}
    		
    }
     
   	/*------------------------------------------------------*
   	 * ---------------VideoThread-----------------*
   	 *------------------------------------------------------*/
     class VideoThread extends Thread{
    		
    	 private boolean m_stop = false;
    	 private int index = 0;
    		
    	 @Override
    	 public void run() {
    		 Log.e("lifecycle","videoscreen");
    		 while(true){
    			 if(m_stop) {
    				 m_stop = false;
    				 break;
    			}
    			 index = Ffmpeg.dqbuf(pixel_save);
    			 if(index < 0) {
    				 //onDestroy();
    				 break;
    			}
    			 
    			screennum++;
    			
    			if(screennum>screennums){
    				Ffmpeg.yuvtorgb(pixel_save, pixel_screen, screen_width, screen_height);
    				screennum=0;
    			}
    				
    				// 向主线程发送更新消息
    			Message message = new Message();
    			message.what = Webcam.UPDATE_VIDEO;
    			handler.sendMessage(message);
    			 
    			 Ffmpeg.qbuf(index); 
    	    }
    	}
    }
     
  	/*------------------------------------------------------*
  	 * ---------------SvideoThread-----------------*
  	 *------------------------------------------------------*/
     public class SvideoThread extends Thread {

    	 @Override
    	 public void run() {
    		 Log.e("lifecycle","videosave");
    		 while(true){
    			 if(!en_video) {
    				 record_stop = true;
    				 break;
    			}
/*    			 Date[5] = EverySecService.psecond;
    			 Date[4] = EverySecService.pminute + Mark_minute;
    			 if (Date[4] >= 60) {
    				 Date[4] -= 60;
    				 Date[3] ++;
    			 }
    			 Date[3] = EverySecService.phour + Mark_hour;*/
    			 Ffmpeg.videostart(pixel_save,Date,pGps);
    		}
    	}
    		
    }
     
 	/*------------------------------------------------------*
 	 * ---------------FreeSizeThread-----------------*
 	 *------------------------------------------------------*/
     class FreeSizeThread extends Thread {

       boolean flag = false;
       @Override
       public void run() {
    	   while(flag != true && en_video == false) {
    		   StatFs sf = new StatFs(SDpath);
    		   //获取单个数据块的大小(Byte)  
    		   long blockSize = sf.getBlockSize();
    		   //空闲的数据块的数量  
    		   long freeBlocks = sf.getAvailableBlocks();
    			//freeBlocks * blockSize;  //单位Byte  
    		   if( (freeBlocks * blockSize)/1024 /1024 < 200 ) {
    				Message message = new Message();
    				message.what = SHORT_OF_SIZE;
    				handler.sendMessage(message);
    				flag = true;
    			}
    	    }
    	   
    	   flag = false;
       }
    }
     
 	/*------------------------------------------------------*
 	 * ---------------屏幕刷新----------------*
 	 *------------------------------------------------------*/
 	public Handler handler = new Handler() {
 		
 		public void handleMessage(Message msg) {
 			switch (msg.what) {
 			case UPDATE_VIDEO:
 			    bitmap.copyPixelsFromBuffer(Imagbuf);
 			    Video_Image.setImageBitmap(bitmap);
 			    Imagbuf.clear();
 			    break;
 			case SHORT_OF_SIZE:
 				Toast.makeText(Webcam.this , "容量小于200MB", Toast.LENGTH_SHORT).show();
 				break;
 			case GPS_DIALOG:
				progressDialog.setMessage("Searching for GPS. . .");
				progressDialog.setCancelable(true);
				progressDialog.show();
 				break;
 			case GPS_CANCEL_DIALOG:
 				progressDialog.dismiss();
 				Toast.makeText(Webcam.this, "开始录制", Toast.LENGTH_SHORT).show();
 			default:
 				break;
 			}
 		}
 	};
    
	/*------------------------------------------------------*
	 * ---------------AlarmReceiver2-----------------*
	 *------------------------------------------------------*/
    class AlarmReceiver2 extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {   
        	Log.e("lifecycle","searchGPSfailed");
        	progressDialog.dismiss();
        	Toast.makeText(Webcam.this, "Can not get in touch. . .	", Toast.LENGTH_SHORT).show();
        	onDestroy();
        }
        
    }
    
	/*------------------------------------------------------*
	 * ---------------AlarmReceiver3-----------------*
	 *------------------------------------------------------*/
    class AlarmReceiver3 extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {    
        	
        	// 隔0.1s 重新开始录制视频
			AlarmManager alarmmanager4 = (AlarmManager) getSystemService(ALARM_SERVICE);
	        int triggertime4 = 100; // 0.1s
	        long triggerAtTime4 = SystemClock.elapsedRealtime() + triggertime4;  // 唤醒时刻 = 系统时间 + triggertime
	        Intent i = new Intent("com.example.giscamera.ALARM4");  // 自定义动作ALARM4
	        PendingIntent pi4 = PendingIntent.getBroadcast(Webcam.this, 0, i, 0);  // 注册广播
	        alarmmanager4.set(AlarmManager.ELAPSED_REALTIME_WAKEUP, triggerAtTime4, pi4);  // 启动AlarmManager
	        
			Log.d(TAG,"++++++++++++stop");
			en_video = false;
			if(locationChanged) {
				while(!record_stop);
				record_stop = false;				
				Ffmpeg.videoclose();
				// 停止定时后台服务
                Intent stopIntent = new Intent(Webcam.this, EverySecService.class);
                stopService(stopIntent);
			}
			Toast.makeText(Webcam.this, "结束录制", Toast.LENGTH_SHORT).show();
			screen_pressed=true;	
			locationChanged = false;
			Log.e("lifecycle","stop_record2");
			//progressDialog.dismiss();

        }
        
    }
    
	/*------------------------------------------------------*
	 * ---------------AlarmReceiver4-----------------*
	 *------------------------------------------------------*/
    class AlarmReceiver4 extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {    	
			Log.d(TAG,"++++++++++++start");
			en_video = true;
			mtime.setToNow();
			
/*			videodir = SDpath + "/DCMI/"+ mtime.year + "_"+ (mtime.month+1) + "_" + mtime.monthDay + "_"
					+ mtime.hour + "_" + mtime.minute + "_" +mtime.second+".avi";
			txtdir = SDpath + "/DCMI/"+ mtime.year + "_"+ (mtime.month+1) + "_" + mtime.monthDay + "_"
					+ mtime.hour + "_" + mtime.minute + "_" +mtime.second+".txt";*/
			
			Log.e("lifecycle","start_record2");
			screen_pressed=false;
			
			new GPSwatch().start();
        }
        
    }
    
	/*------------------------------------------------------*
	 * ---------------VideoListener-----------------*
	 *------------------------------------------------------*/
	private OnClickListener videolistener = new OnClickListener(){

		@Override
		public void onClick(View v) {
			if(screen_pressed) {
				Log.e("lifecycle","start_record");
				en_video = true;
				//mtime.setToNow();
			
				// 判断sd卡位置
				if (!savedir) {
					if(SdCardUtils.isSecondSDcardMounted()) {
						SDpath = SdCardUtils.getSecondExterPath();
						Toast.makeText(Webcam.this, "外置SD卡", Toast.LENGTH_SHORT).show();
						}
					else if(SdCardUtils.isFirstSdcardMounted()) {
						SDpath = SdCardUtils.getFirstExterPath();
						Toast.makeText(Webcam.this, "内置存储", Toast.LENGTH_SHORT).show();
						}
					else {
						Toast.makeText(Webcam.this, "无存储介质", Toast.LENGTH_SHORT).show();
						onDestroy();
						}
				}
				else {
					if(SdCardUtils.isFirstSdcardMounted()) {
						SDpath = SdCardUtils.getFirstExterPath();
						Toast.makeText(Webcam.this, "内置存储", Toast.LENGTH_SHORT).show();
						}
					else if(SdCardUtils.isSecondSDcardMounted()) {
						SDpath = SdCardUtils.getSecondExterPath();
						Toast.makeText(Webcam.this, "外置SD卡", Toast.LENGTH_SHORT).show();
						}
					else {
						Toast.makeText(Webcam.this, "无存储介质", Toast.LENGTH_SHORT).show();
						onDestroy();
						}
				}

				// 判断是否存在DCMI文件夹，若无则进行创建
				File videofile = new File(SDpath + "/DCMI/");
				  if (!videofile.exists()) {
					   videofile.mkdir();
				 }
/*				videodir = SDpath + "/DCMI/gm"+ mtime.year + (mtime.month+1) + mtime.monthDay + "_"
						+ mtime.hour + mtime.minute + mtime.second+".avi";
				
				txtdir = SDpath + "/DCMI/gm"+ mtime.year + (mtime.month+1) + mtime.monthDay + "_"
						+ mtime.hour + mtime.minute + mtime.second+".txt";*/
				
				//videodir = "/mnt/sdcard/DCMI/gm20120101_072806.avi";
				
				screen_pressed=false;
				//Log.i("1",videodir);
				new GPSwatch().start();  // 开始录制线程
				
			}
			else {
				Log.e("lifecycle","stop_record");
				en_video = false;
				if(locationChanged) {
					while(!record_stop);
					record_stop = false;				
					Ffmpeg.videoclose();
					// 停止定时后台服务
	                Intent stopIntent = new Intent(Webcam.this, EverySecService.class);
	                stopService(stopIntent);
				}
				Toast.makeText(Webcam.this, "结束录制", Toast.LENGTH_SHORT).show();
				locationChanged = false;
				screen_pressed=true;
			}		
		}
	};

	/*------------------------------------------------------*
	 * ---------------位置监听----------------*
	 *------------------------------------------------------*/
	private LocationListener myLocationListener = new LocationListener() {			

        public void onProviderDisabled(final String s) {
      	  Log.d(TAG, "onProviderDisabled. ");
		//  Toast.makeText(Webcam.this, "Provider Disabled", Toast.LENGTH_SHORT).show();
        }          


         public void onProviderEnabled(final String s) {
               Log.d(TAG, "onProviderEnabled. ");
        //      Toast.makeText(Webcam.this, "Provider Enabled", Toast.LENGTH_SHORT).show();
         }

         public void onStatusChanged(String provider,  int status, Bundle extras) {
               Log.d(TAG, "onStatusChanged. ");
             //Toast.makeText(Webcam.this, "Status Changed", Toast.LENGTH_SHORT).show();
         }
         
        @Override
        public void onLocationChanged(Location loc) {
      	  Log.d("locationchanged", "onLocationChanged. loc: " + loc);	
      	  
      	  if (loc != null) {      		  
      		  sharelocation = loc;
      		  
      		  utctime = loc.getTime() + 28800000;  // 北京时间=UTC+8小时
      		  calendar.setTimeInMillis(utctime);
      		  Date[0] = calendar.get(Calendar.YEAR)%100; //获取当前年份 
      		  Date[1] = calendar.get(Calendar.MONTH) + 1;//获取当前月份 
      		  Date[2] = calendar.get(Calendar.DAY_OF_MONTH);//获取当前月份的日期号码 
      		  //Date[3] = calendar.get(Calendar.HOUR_OF_DAY);//获取当前的小时数 
      		  //Date[4] = calendar.get(Calendar.MINUTE);//获取当前的分钟数 
      		  //Date[5] =	calendar.get(Calendar.SECOND); 
      		  if(locationChanged == false) {
/*      			  Mark_hour = calendar.get(Calendar.HOUR_OF_DAY);
      			  Mark_minute = calendar.get(Calendar.MINUTE);
      			  Mark_second = calendar.get(Calendar.SECOND);*/
          		  Date[3] = calendar.get(Calendar.HOUR_OF_DAY);//获取当前的小时数 
          		  Date[4] = calendar.get(Calendar.MINUTE);//获取当前的分钟数 
          		  Date[5] =	calendar.get(Calendar.SECOND); 
      		  }
      		  pGps[0] = loc.getLongitude();
      		  pGps[1] = loc.getLatitude();
      		  pGps[2] = loc.getAltitude();
      		  
      		  // 更新屏幕文本
      		  text_view_latitude.setText("经度：" + pGps[0]);
      		  text_view_longtitude.setText("纬度：" + pGps[1]);
      		  text_view_altitude.setText("海拔：" + pGps[2]);
      		  text_view_speed.setText("速度：" + loc.getSpeed());
      		  
      		  locationChanged = true;
          }
      	  else {
      		  Toast.makeText(Webcam.this, "Your current location is temporarily unavailable.", Toast.LENGTH_SHORT).show();
          }
       }
     };
     
	@Override
	public void onDestroy() {
		Ffmpeg.release();
		lm.removeUpdates(myLocationListener);
		super.onDestroy();		
	}

	@Override
	protected void onPause() {
		super.onPause();
		unregisterReceiver(alarmreceiver2);  // 动态注销广播接收器
		unregisterReceiver(alarmreceiver3);  // 动态注销广播接收器
		unregisterReceiver(alarmreceiver4);  // 动态注销广播接收器
		//onDestroy();
		Log.e("lifecycle","onPause");
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		registerReceiver(alarmreceiver2, intentFilter2);  // 动态注册广播接收器
		registerReceiver(alarmreceiver3, intentFilter3);  // 动态注册广播接收器
		registerReceiver(alarmreceiver4, intentFilter4);  // 动态注册广播接收器
		Log.e("lifecycle","onResume");
	}
	
}
