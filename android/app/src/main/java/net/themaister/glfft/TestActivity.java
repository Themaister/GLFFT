package net.themaister.glfft;

import android.os.AsyncTask;
import android.os.Bundle;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.concurrent.CancellationException;
import java.util.concurrent.ExecutionException;

public class TestActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        ScrollView scrollView = (ScrollView) findViewById(R.id.scrollView);
        scrollView.fullScroll(View.FOCUS_DOWN);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_test, menu);
        return true;
    }

    private void runTestSuiteInThread() {
        endCurrentRun();
        currentTask = new GLFFTTask((TextView) findViewById(R.id.content));
        currentTask.execute(GLFFTTask.FULL_TEST_SUITE);
    }

    private void runBasicBenchInThread() {
        endCurrentRun();
        currentTask = new GLFFTTask((TextView) findViewById(R.id.content));
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

        TextView view;

        GLFFTTask(TextView textView) {
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

            result = Native.getExitCode();
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
            view.append(progress[0]);
        }

        @Override
        protected void onPreExecute() {
            view.append("\nStarting GLFFT task ...\n");
        }

        @Override
        protected void onPostExecute(Integer result) {
            view.append("GLFFT Task completed! (code: " + result + ")\n\n");
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
        if (currentTask != null) {
            currentTask.cancel(false);
            currentTask = null;
        }
    }
}
