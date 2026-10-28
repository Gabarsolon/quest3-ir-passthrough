package com.gabarsolon.questir;

import android.app.Activity;
import android.app.Instrumentation;
import android.os.Bundle;

/** Invoked through authorized ADB instrumentation; never runs an exploit or hooks. */
public class SmokeTest extends Instrumentation {
    @Override public void onCreate(Bundle args) { super.onCreate(args); start(); }
    @Override public void onStart() {
        Bundle result = new Bundle();
        try {
            String report = MainActivity.root("id; /dev/quest3-ir-session/headset/ir-headset --check-only");
            if (!report.contains("uid=0(root)") || !report.contains("No hooks installed"))
                throw new Exception("App could not verify temporary root and native preflight: " + report);
            result.putString("stream", "App uid " + android.os.Process.myUid() + " successfully called temporary root.\n" + report + "\n");
            finish(Activity.RESULT_OK, result);
        } catch (Exception error) {
            result.putString("stream", error.toString() + "\n"); finish(Activity.RESULT_CANCELED, result);
        }
    }
}
