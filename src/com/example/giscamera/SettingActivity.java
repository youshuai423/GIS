package com.example.giscamera;

import android.app.Activity;
import android.os.Bundle;
import android.view.Window;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Switch;
import android.widget.Toast;

public class SettingActivity extends Activity {

	RadioGroup m_RadioGroup;
    RadioButton neverRadio, Radio5, Radio15, Radio30; 
    Switch mSwitch;
    
	@Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		super.requestWindowFeature(Window.FEATURE_NO_TITLE);
        //super.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);  // 全屏
        setContentView(R.layout.settingactivity);
        
        m_RadioGroup = (RadioGroup) findViewById(R.id.save_interval);  
        neverRadio = (RadioButton) findViewById(R.id.neversave);
        Radio5 = (RadioButton) findViewById(R.id.s5min);  
        Radio15 = (RadioButton) findViewById(R.id.s15min);  
        Radio30 = (RadioButton) findViewById(R.id.s30min);  
        mSwitch = (Switch) findViewById(R.id.internal_storage);

        m_RadioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {  
        	@Override  
        	public void onCheckedChanged(RadioGroup group, int checkedId)  
        	{  
        		if (checkedId == neverRadio.getId())  {
        			Webcam.save_interval = 0;
        		}
        		else if (checkedId == Radio5.getId()) {
        			Webcam.save_interval = 1;
        		}
        		else if (checkedId == Radio15.getId()) {
        			Webcam.save_interval = 15;
        		}
        		else if (checkedId == Radio30.getId()) {
        			Webcam.save_interval = 30;
        		}
        		else {
        			Toast.makeText(SettingActivity.this, "存储时间设置失败", Toast.LENGTH_SHORT).show();
        		}
            }  
        }); 
        
        mSwitch.setOnCheckedChangeListener(new OnCheckedChangeListener() {

			@Override
			public void onCheckedChanged(CompoundButton arg0, boolean isChecked) {
				if	(isChecked) {
					Webcam.savedir = true;
				}
				else {
					Webcam.savedir = false;
				}
			}
        });

	}
}
