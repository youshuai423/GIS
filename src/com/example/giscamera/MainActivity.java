package com.example.giscamera;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import android.view.View;
import android.view.Window;
import android.widget.ImageButton;
import android.widget.Toast;

public class MainActivity extends Activity {
	private ImageButton btn1 = null;// camera
	private ImageButton btn2 = null;// 卸载SD
	private ImageButton btn3 = null;// GPS
	private ImageButton btn4 = null;

	@Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		super.requestWindowFeature(Window.FEATURE_NO_TITLE);
        //super.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);  // 全屏
        setContentView(R.layout.activity_main);

        btn1 = (ImageButton) findViewById(R.id.imageButton1);  // GIS
        btn2 = (ImageButton) findViewById(R.id.imageButton2);  // SD
        btn3 = (ImageButton) findViewById(R.id.imageButton3);  // SETTING
        btn4 = (ImageButton) findViewById(R.id.imageButton4);  // TIMESET
        
        // 修改系统时间（待修改）
/*        try{
        	SystemDateTime.setDateTime(2016, 1, 28, 20, 0);
        }
        catch (IOException ee) {
        	
        }
        catch (InterruptedException ii) {
        	
        }*/
        

/*******************************************************************************************************************/        
        btn1.setOnClickListener(new ImageButton.OnClickListener(){

			@Override
			public void onClick(View v) {

				Intent intent = new Intent();
				intent.setClass(MainActivity.this, Webcam.class);
				startActivity(intent);	
			}
        	
        });
        
        btn2.setOnClickListener(new ImageButton.OnClickListener(){

			@Override
			public void onClick(View v) {
				if(SdCardUtils.isSecondSDcardMounted()) {
					String SDpath = SdCardUtils.getSecondExterPath();
					unmount(SDpath);
					Toast.makeText(MainActivity.this, "SD卡已安全弹出", Toast.LENGTH_SHORT).show();
				}
				else {
					Toast.makeText(MainActivity.this, "未安装SD卡", Toast.LENGTH_SHORT).show();
				}
			}       	
        });
        
        btn3.setOnClickListener(new ImageButton.OnClickListener(){

			@Override
			public void onClick(View v) {
				Intent intent = new Intent(MainActivity.this, SettingActivity.class);  // 设置功能
				startActivity(intent);
			}       	
        });
        
        btn4.setOnClickListener(new ImageButton.OnClickListener(){

			@Override
			public void onClick(View v) {
				
				Intent intent = new Intent(Settings.ACTION_DATE_SETTINGS);  // 系统时间设定
				startActivity(intent);

			}       	
        });
    }
	
	// 弹出SD卡
    private boolean unmount(String volume) {
        try {
            Method getService = Class.forName("android.os.ServiceManager").getMethod("getService", String.class);
            Object service = getService.invoke(null, "mount");
 
            Class<?> Stub = Class.forName("android.os.storage.IMountService$Stub");
 
            Method asInterface = Stub.getMethod("asInterface", Class.forName("android.os.IBinder"));
            Object mountServ = asInterface.invoke(null, service);
 
            Method unmountVolume = mountServ.getClass().getMethod("unmountVolume", String.class, boolean.class, boolean.class);
            Object result = unmountVolume.invoke(mountServ, volume, true, false);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
 
        return false;
    }

}
