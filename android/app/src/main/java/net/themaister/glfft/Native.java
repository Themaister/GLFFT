package net.themaister.glfft;

public class Native {
    native static int runTestSuite();
    static {
        System.loadLibrary("GLFFT");
    }
}
