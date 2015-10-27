package net.themaister.glfft;

import android.content.Context;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.PowerManager;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CancellationException;
import java.util.concurrent.ExecutionException;

public class TestActivity extends AppCompatActivity {

    PowerManager.WakeLock wakeLock;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        wakeLock = pm.newWakeLock(PowerManager.ACQUIRE_CAUSES_WAKEUP | PowerManager.FULL_WAKE_LOCK, "GLFFT");
        wakeLock.acquire();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_test, menu);
        return true;
    }

    private void runTestSuiteInThread() {
        endCurrentRun();
        currentTask = new GLFFTTask((ScrollView) findViewById(R.id.scrollView), (TextView) findViewById(R.id.content));
        currentTask.execute(GLFFTTask.FULL_TEST_SUITE);
    }

    private void runBasicBenchInThread() {
        endCurrentRun();
        currentTask = new GLFFTTask((ScrollView) findViewById(R.id.scrollView), (TextView) findViewById(R.id.content));
        currentTask.execute(GLFFTTask.BASIC_BENCH);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        switch (id) {
            case R.id.action_run_test_suite:
                runTestSuiteInThread();
                return true;

            case R.id.action_basic_bench:
                runBasicBenchInThread();
                return true;
        }

        return super.onOptionsItemSelected(item);
    }

    private class GLFFTTask extends AsyncTask<Integer, String, Integer> {
        public static final int FULL_TEST_SUITE = 1;
        public static final int BASIC_BENCH = 2;

        private TextView view;
        private ScrollView sview;
        private List<String> logOutput = new ArrayList<>();
        private static final int MAX_LINES = 256;

        private void appendLog(String str) {
            logOutput.add(str);
            if (logOutput.size() > MAX_LINES)
                logOutput.remove(0);
            updateLog();
        }

        private void clearLog() {
            logOutput.clear();
            updateLog();
        }

        private void updateLog() {
            StringBuilder builder = new StringBuilder();
            for (String str : logOutput)
                builder.append(str);

            view.setText(builder.toString());
            sview.fullScroll(View.FOCUS_DOWN);
        }

        public GLFFTTask(ScrollView scrollView, TextView textView) {
            sview = scrollView;
            view = textView;
        }

        @Override
        protected Integer doInBackground(Integer... args) {
            int result = 0;
            switch (args[0]) {
                case FULL_TEST_SUITE:
                    result = Native.beginRunTestSuiteTask();
                    break;

                case BASIC_BENCH:
                    result = Native.beginBenchTask();
                    break;
            }

            if (result != 0) {
                Native.endTask();
                return result;
            }

            String log = Native.pull();
            while (log != null || Native.isComplete() == 0) {
                publishProgress(log);
                if (isCancelled())
                    break;
                log = Native.pull();
            }

            result = isCancelled() ? 0 : Native.getExitCode();
            Native.endTask();
            return result;
        }

        @Override
        protected void onCancelled(Integer result) {
            view.append("GLFFT Task cancelled! (code: " + result + ")\n");
            Snackbar.make(findViewById(R.id.content), "GLFFT run cancelled.", Snackbar.LENGTH_LONG).show();
        }

        @Override
        protected void onProgressUpdate(String... progress) {
            appendLog(progress[0]);
        }

        @Override
        protected void onPreExecute() {
            clearLog();
            appendLog("\nStarting GLFFT task ...\n");
            sview.fullScroll(View.FOCUS_DOWN);
        }

        @Override
        protected void onPostExecute(Integer result) {
            view.append("GLFFT Task completed! (code: " + result + ")\n\n");
            sview.fullScroll(View.FOCUS_DOWN);
            Snackbar.make(findViewById(R.id.content), "Ran GLFFT ... exit code: " + result, Snackbar.LENGTH_LONG).show();
        }
    }

    private GLFFTTask currentTask = null;

    private void endCurrentRun() {
        if (currentTask != null) {
            currentTask.cancel(false);
            try {
                currentTask.get();
            } catch (CancellationException e) {
            } catch (ExecutionException e) {
            } catch (InterruptedException e) {
            }
            currentTask = null;
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (wakeLock != null) {
            wakeLock.release();
            wakeLock = null;
        }

        if (currentTask != null) {
            currentTask.cancel(false);
            currentTask = null;
        }
    }
}
