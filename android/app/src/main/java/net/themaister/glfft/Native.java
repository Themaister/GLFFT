package net.themaister.glfft;

public class Native {
    native static int beginRunTestSuiteTask();
    native static int beginBenchTask();
    native static int iterate();
    native static int endTask();
    native static int getCurrentProgress();
    native static int getTargetProgress();
    static {
        System.loadLibrary("GLFFT");
    }
}
