package com.example.giscamera;

/*--------------------------------------------------------
 * ---------------------Service---------------------------
*-----------产生一个1s的AlarmManager---------------
*----------每次启动将GPS信息写入txt文件--------------
*--------------------------------------------------------*/

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import com.example.giscamera.AlarmReceiver;
import com.example.giscamera.Webcam;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.SystemClock;
import android.text.format.Time;
import android.util.Log;

public class EverySecService extends Service {

	private File file;
	private FileOutputStream out=null; 
	private AlarmManager alarmmanager;
	private PendingIntent pi;  // 广播事件
	private Time time = new Time();
	private int cyear = 2016, cmonth = 1, cday = 1;
	private int chour = 0, cminute = 0, csecond = 0;  // 运行时刻
	private int count = 0; // 运行秒数
	
	public static int phour = 0, pminute = 0, psecond = 0;  // 录制时间
	
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
    
    @Override
    public void onCreate() {
    	super.onCreate();
    	String txtfile = Webcam.txtdir;
    	file = new File(txtfile);
    	time.setToNow();

    	// 取得输出txt文件的写入对象
    	try {
    		out = new FileOutputStream(file); 
    	}
    	catch (IOException e) {
    		e.printStackTrace();
    	}
    	
    	Log.d("servvvv", "create");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int starId) {
    	
    	timecal(count);  // 计算录制时间
    	
    	new Thread(new Runnable() {
    		//switch
    		String output = null;
    		@Override
    		public void run() {
    			try {
    				//time.setToNow();
    				cyear = Webcam.Date[0];
    				cmonth = Webcam.Date[1];
    				cday = Webcam.Date[2];
    				chour = Webcam.Date[3];
    				cminute = Webcam.Date[4];
    				csecond = Webcam.Date[5];
    				
    				output = "" + cyear%100 + "-" + cmonth + "-" + cday + "    " + chour + ":" + cminute + ":" + csecond + "    " 
    						+ (double)(Math.floor(Webcam.sharelocation.getLongitude()*1000000))/1000000.0 + "°	" 
    						+ (double)(Math.floor(Webcam.sharelocation.getLatitude()*1000000))/1000000.0 + "°" + "\r\n";
    					
    //------------------------txt输出-----------------------------
    				out.write(output.getBytes());
    //--------------------------------------------------------------					
    					
    				count++;
    				}
    			catch (IOException e) {
    				e.printStackTrace();
    				}
    			}
    		}).start();
    	
    	//每秒出发Alarm一次
    	alarmmanager = (AlarmManager) getSystemService(ALARM_SERVICE);
    	int Second = 1 * 1000; // 1s
    	long triggerAtTime = SystemClock.elapsedRealtime() + Second;  // 唤醒时刻 = 系统时间 + 1s
    	Intent i = new Intent(this, AlarmReceiver.class);
    	pi = PendingIntent.getBroadcast(this, 1, i, 0);  // 注册广播
    	alarmmanager.set(AlarmManager.ELAPSED_REALTIME_WAKEUP, triggerAtTime, pi);
    	return super.onStartCommand(intent, flags, starId);
    }
    
    public void onDestroy() {
    	super.onDestroy();
    	alarmmanager.cancel(pi);
    	try {
    		if(out != null) {
    			out.close();
    		}
    	}
    	catch (IOException e) {
    		e.printStackTrace();
    	}
    	Log.d("servvvv", "stop");
    }

    // 计算运行时间
    private void timecal(int count) {	
    	phour = count / (60*60);
    	pminute = count / (60);
    	psecond = count % 60;    	
    	Webcam.text_view_time.setText(phour + ":" + pminute + ":" + psecond);
    	
    	Webcam.Date[5]++;
    	if(Webcam.Date[5]>=60) {
    		Webcam.Date[5] -= 60;
    		Webcam.Date[4]++;
    	}
    	if (Webcam.Date[4] >= 60) {
    		Webcam.Date[4] -= 60;
    		Webcam.Date[3] ++;
    	}
    }
}
