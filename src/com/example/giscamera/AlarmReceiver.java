package com.example.giscamera;

/*---------------------------------------------
 * ---接受AlarmManager每秒发送的广播----
 * ----------------------------------------------*/

import com.example.giscamera.EverySecService;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class AlarmReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {    	
        Intent i = new Intent(context, EverySecService.class);
        context.startService(i);  // 重新启动服务
    }
    
}
