package com.example.giscamera;

/*---------------------------------------------
 * ---����AlarmManagerÿ�뷢�͵Ĺ㲥----
 * ----------------------------------------------*/

import com.example.giscamera.EverySecService;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class AlarmReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {    	
        Intent i = new Intent(context, EverySecService.class);
        context.startService(i);  // ������������
    }
    
}
