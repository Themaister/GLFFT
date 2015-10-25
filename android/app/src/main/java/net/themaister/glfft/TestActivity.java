package net.themaister.glfft;

import android.os.AsyncTask;
import android.os.Bundle;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;

public class TestActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_test, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        if (id == R.id.action_run_test_suite) {
            runTestSuiteInThread();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    private class GLFFTTask extends AsyncTask<Integer, Integer, Integer> {
        public static final int FULL_TEST_SUITE = 1;

        @Override
        protected Integer doInBackground(Integer... args) {
            int result = 0;
            switch (args[0]) {
                case FULL_TEST_SUITE:
                    // TODO: Need to have proper progress bars.
                    result = Native.runTestSuite();
                    break;
            }

            return Integer.valueOf(result);
        }

        //@Override
        //protected void onProgressUpdate(Integer... progress) {
            //setProgressPercent(progress[0]);
        //}

        @Override
        protected void onPostExecute(Integer result) {
            Snackbar.make(findViewById(R.id.content), "Ran GLFFT ... exit code: " + result, Snackbar.LENGTH_LONG).show();
        }
    }

    private void runTestSuiteInThread() {
        new GLFFTTask().execute(GLFFTTask.FULL_TEST_SUITE);
    }
}
