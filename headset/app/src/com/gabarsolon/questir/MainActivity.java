package com.gabarsolon.questir;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.graphics.Color;
import android.view.View;
import android.widget.*;
import java.io.*;
import java.util.Locale;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicBoolean;
import org.json.JSONObject;

/** Small controller UI. The root process owns IR, independently of this activity. */
public class MainActivity extends Activity {
    private static final String BASE = "/dev/quest3-ir-session/headset";
    private final ExecutorService worker = Executors.newSingleThreadExecutor();
    private final Handler handler = new Handler();
    private final AtomicBoolean busy = new AtomicBoolean();
    private TextView state, detail;
    private EditText brightness;
    private Spinner filter;
    private CheckBox display, aa;
    private boolean resumed;

    @Override public void onCreate(Bundle saved) {
        super.onCreate(saved);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL); layout.setPadding(40, 30, 40, 30);
        layout.setBackgroundColor(Color.rgb(22, 26, 32));
        ScrollView scroll = new ScrollView(this); scroll.addView(layout); setContentView(scroll);
        TextView title = label("Quest IR", 28); layout.addView(title);
        state = label("Checking headset controller…", 20); layout.addView(state);
        layout.addView(label("Temporary root must already be active. Reboot clears root and IR patches.\nYou can close this app after starting IR.", 15));
        layout.addView(label("Brightness (0–4)", 16));
        brightness = new EditText(this); brightness.setText("1.0"); brightness.setSingleLine(true);
        brightness.setTextColor(Color.WHITE); brightness.setInputType(8194); layout.addView(brightness);
        layout.addView(label("Reconstruction filter", 16));
        filter = new Spinner(this);
        ArrayAdapter<String> choices = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item,
                new String[]{"linear", "cubic", "sgsr"});
        filter.setAdapter(choices); layout.addView(filter);
        aa = new CheckBox(this); aa.setText("Edge anti-aliasing"); aa.setTextColor(Color.WHITE); layout.addView(aa);
        display = new CheckBox(this); display.setText("120 Hz display (camera remains about 60 FPS)");
        display.setTextColor(Color.WHITE); layout.addView(display);
        Button start = new Button(this); start.setText("Start IR"); layout.addView(start); start.setOnClickListener(v -> startIR());
        Button stop = new Button(this); stop.setText("Stop IR / restore RGB"); layout.addView(stop);
        stop.setOnClickListener(v -> task(() -> root("if [ -x " + BASE + "/ir-headset ]; then " + BASE + "/ir-headset --stop; else echo 'IR is not staged'; fi")));
        detail = label("Options apply to the next run. New binaries, frames and logs stay in RAM.", 14); layout.addView(detail);
    }

    private TextView label(String text, int size) {
        TextView view = new TextView(this); view.setText(text); view.setTextSize(size);
        view.setTextColor(Color.rgb(225, 233, 242)); view.setPadding(0, 10, 0, 12); return view;
    }
    private static String quote(String value) { return "'" + value.replace("'", "'\"'\"'") + "'"; }
    static String root(String command) throws Exception {
        Process process = new ProcessBuilder("/data/local/tmp/su", "-c", command).redirectErrorStream(true).start();
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        Thread reader = new Thread(() -> {
            try (InputStream input = process.getInputStream()) {
                byte[] block = new byte[4096]; int size;
                while ((size = input.read(block)) >= 0) output.write(block, 0, size);
            } catch (IOException ignored) { }
        });
        reader.setDaemon(true); reader.start();
        if (!process.waitFor(20, TimeUnit.SECONDS)) {
            process.destroyForcibly(); reader.join(1000); throw new IOException("Root command timed out");
        }
        reader.join(1000);
        String text = output.toString("UTF-8").trim();
        if (process.exitValue() != 0) throw new IOException(text.isEmpty() ? "Temporary root is unavailable" : text);
        return text;
    }
    private interface Job { String run() throws Exception; }
    private void task(Job job) {
        if (!busy.compareAndSet(false, true)) { detail.setText("Checking status; tap again in a moment."); return; }
        worker.execute(() -> {
            try { String result = job.run(); handler.post(() -> detail.setText(result)); }
            catch (Exception error) { handler.post(() -> detail.setText(error.getMessage())); }
            finally { busy.set(false); }
        });
    }
    private String stage(String name, String digest) {
        String path = BASE + "/" + name;
        return "if [ \"$(sha256sum " + path + " 2>/dev/null | cut -d' ' -f1)\" != " + quote(digest) + " ]; then "
                + "unzip -p " + quote(getApplicationInfo().sourceDir) + " assets/" + name + " > " + path + ".new || exit 1; "
                + "test \"$(sha256sum " + path + ".new | cut -d' ' -f1)\" = " + quote(digest) + " || exit 1; "
                + "chmod 0700 " + path + ".new && mv " + path + ".new " + path + " || exit 1; fi; ";
    }
    private void startIR() {
        final double value;
        try { value = Double.parseDouble(brightness.getText().toString()); }
        catch (Exception error) { detail.setText("Enter brightness from 0 to 4."); return; }
        if (Double.isNaN(value) || Double.isInfinite(value) || value < 0 || value > 4) { detail.setText("Enter brightness from 0 to 4."); return; }
        String args = " --brightness " + String.format(Locale.ROOT, "%.5f", value)
                + " --filter " + quote(filter.getSelectedItem().toString())
                + (aa.isChecked() ? " --aa edge" : "") + (display.isChecked() ? " --display-refresh 120" : "");
        task(() -> {
            String command = "test \"$(id -u)\" = 0 || exit 1; "
                    + "test \"$(stat -f -c %t /dev)\" = 1021994 || exit 1; "
                    + "test \"$(getprop ro.build.version.incremental)\" = 51154110174900520 || exit 1; "
                    + "if [ -x " + BASE + "/ir-headset ]; then "
                    + BASE + "/ir-headset --status | grep -q '\"status\":\"stopped\"' || { echo 'Stop the running controller before starting again'; exit 1; }; fi; "
                    + "mkdir -p " + BASE + " && chmod 0700 " + BASE + " || exit 1; "
                    + stage("irfeed", BuildInfo.READER_SHA256) + stage("ir-headset", BuildInfo.CONTROLLER_SHA256)
                    + "TMPDIR=" + BASE + " setsid " + BASE + "/ir-headset" + args
                    + " </dev/null >" + BASE + "/controller.log 2>&1 &";
            root(command); return "Starting IR on the headset…";
        });
    }
    private final Runnable poll = new Runnable() {
        public void run() {
            if (!resumed) return;
            if (busy.compareAndSet(false, true)) worker.execute(() -> {
                try {
                    String text = root("if [ -x " + BASE + "/ir-headset ]; then " + BASE + "/ir-headset --status; else echo '{\"status\":\"ready to stage\"}'; fi");
                    JSONObject obj = new JSONObject(text); String status = obj.optString("status", "Unknown");
                    handler.post(() -> state.setText(status));
                } catch (Exception error) { handler.post(() -> state.setText("Temporary root unavailable")); }
                finally { busy.set(false); }
            });
            handler.postDelayed(this, 3000);
        }
    };
    @Override public void onResume() { super.onResume(); resumed=true; handler.post(poll); }
    @Override public void onPause() { resumed=false; handler.removeCallbacks(poll); super.onPause(); }
    @Override public void onDestroy() { worker.shutdown(); super.onDestroy(); }
}
