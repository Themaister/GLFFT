package net.themaister.glfft;

public class Native {
    native static int beginRunTestSuiteTask();
    native static int beginBenchTask();
    native static void endTask();

    native static int isComplete();
    native static int getExitCode();
    native static String pull();

    static {
        System.loadLibrary("GLFFT");
    }
}
