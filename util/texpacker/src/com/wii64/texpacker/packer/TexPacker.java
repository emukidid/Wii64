package com.wii64.texpacker.packer;

import com.wii64.texpacker.util.Log;

/**
 * Rice Hi-Res texture com.wii64.texpacker.packer for Wii64
 *
 * @author emu_kidid
 */

public class TexPacker {


    public static void main(String[] argv) {
        try {
            Log.info("Program startup");
            Archiver.getInstance().setVisible(true);
            while (Archiver.getInstance().isVisible()) {
                Thread.sleep(100);
                if (Archiver.getInstance().isStartPushed()) {
                    Archiver.getInstance().run();
                    Archiver.getInstance().setStartPushed(false);
                }
            }
            Log.info("Program exit");
        } catch (InterruptedException e) {
            Log.info("Cancelled");
        }
    }

}
