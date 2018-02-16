package com.wii64.texpacker.packer;

import org.apache.commons.lang.StringUtils;

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.zip.GZIPOutputStream;

/**
 * A basic archive entry. Contains the actual pixels + meta (conversion from
 * pixel data to GX happens here)
 *
 * @author emu_kidid
 */
class ArchiveEntry {
    private ArchiveEntryMeta metaData;
    private byte[] gxTexture;
    private File primaryFile;
    private File alphaFile;

    // Examples of Hi-Res texture naming
    // SUPER MARIO 64#0B6D2926#0#2_all.png
    // SUPER MARIO 64#D8903B0B#0#2_rgb.png
    // SUPER MARIO 64#9FBECEF9#0#2_a.png
    // WAVE RACE 64#FAF51949#2#1#897BF8BE_ciByRGBA.png (with paletteCRC)

    // If there is a '_all' file, use it and disregard the rest (_rgb,_a)
    // If there is a '_rgb' file, look for a '_a' file for alpha, if not just
    // use '_rgb'

    /**
     * Reads the meta data out of a file name, reads the png pixel data from the
     * file, converts it to a GX texture and GZIP compresses it.
     *
     * @param primaryFile the main image file
     * @param alphaFile   the supplementary alpha file
     */
    ArchiveEntry(File primaryFile, File alphaFile) {
        this.primaryFile = primaryFile;
        this.alphaFile = alphaFile;
    }

    boolean process() {
        int[] primaryPixels;
        int[] alphaPixels = null;
        boolean replaceAlphaWithB = StringUtils.startsWithIgnoreCase(
                StringUtils.substringAfter(StringUtils.substringAfter(primaryFile.getName(), "#"), "#"), "4#");

        metaData = new ArchiveEntryMeta();
        metaData.setFileName(primaryFile.getAbsolutePath());

        // Decompress the primary PNG

        // Grab the primary PNG pixels
        File f = new File(metaData.getFileName());
        BufferedImage bufferedImage;
        try {
            bufferedImage = ImageIO.read(f);
            metaData.setWidth(bufferedImage.getWidth());
            metaData.setHeight(bufferedImage.getHeight());
        } catch (IOException e) {
            metaData.setErrorMsg("Failed to read Image from file! Please check that this is a valid image.");
            return false;
        }
        metaData.setRawPixelType(getType(bufferedImage.getType()));
        primaryPixels = bufferedImage.getRGB(0, 0, metaData.getWidth(), metaData.getHeight(), null, 0, metaData.getWidth());

        // Decompress the alpha PNG if it exists
        BufferedImage alphaImage;
        if (alphaFile != null) {
            try {
                alphaImage = ImageIO.read(alphaFile);
                metaData.setAlphaWidth(alphaImage.getWidth());
                metaData.setAlphaHeight(alphaImage.getHeight());
            } catch (IOException e) {
                metaData.setErrorMsg("Failed to read alpha Image from file! Please check that this is a valid image.");
                return false;
            }
            metaData.setRawAlphaPixelType(getType(alphaImage.getType()));
            alphaPixels = alphaImage.getRGB(0, 0, metaData.getAlphaWidth(), metaData.getAlphaHeight(), null, 0, metaData.getAlphaWidth());
        }

        // Convert 24bit or 24bit + alpha/etc
        if (bufferedImage.getType() == BufferedImage.TYPE_3BYTE_BGR) {
            if (alphaPixels != null && alphaPixels.length != primaryPixels.length) {
                metaData.setErrorMsg("Alpha data cannot differ in dimensions from RGB data, to continue, delete/fix the _a.png file and start the process again");
                return false;
            }
            convertTex24bpp(primaryPixels, alphaPixels, replaceAlphaWithB);
        }
        // Else, simple 32 bit RGBA conversion
        else if (bufferedImage.getType() == BufferedImage.TYPE_4BYTE_ABGR) {
            convertTex32bpp(primaryPixels);
        }

        // Compress it
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        GZIPOutputStream gz;
        try {
            gz = new GZIPOutputStream(bos);
            gz.write(gxTexture);
            gz.flush();
            gz.close();
            gxTexture = bos.toByteArray();
            bos.close();
        } catch (IOException e) {
            metaData.setErrorMsg("Failed to compress converted image! Report this bug to the developer.");
            return false;
        }

        metaData.setCompressedLength(gxTexture.length);

        // Standardise directory separators across platforms
        metaData.setFileName(StringUtils.substringAfterLast(StringUtils.replace(metaData.getFileName(), "\\", "/"), "/"));

        String textureInfo = StringUtils.substringAfter(metaData.getFileName(), "#");

        metaData.setTextureCRC(Long.parseLong(StringUtils.substringBefore(textureInfo, "#"), 16));
        textureInfo = StringUtils.substringAfter(textureInfo, "#");
        metaData.setFormat(Short.parseShort(StringUtils.substringBefore(textureInfo, "#"), 16));
        textureInfo = StringUtils.substringAfter(textureInfo, "#");

        // PaletteCRC might be supplied
        if (StringUtils.contains(textureInfo, "#")) {
            metaData.setSize(Short.parseShort(StringUtils.substringBefore(textureInfo, "#"), 16));
            textureInfo = StringUtils.substringAfter(textureInfo, "#");
            metaData.setPaletteCRC(Long.parseLong(StringUtils.substringBefore(textureInfo, "_"), 16));
            textureInfo = StringUtils.substringAfter(textureInfo, "_");
        } else {
            metaData.setPaletteCRC((long) -1);
            metaData.setSize(Short.parseShort(StringUtils.substringBefore(textureInfo, "_"), 16));
            textureInfo = StringUtils.substringAfter(textureInfo, "_");
        }

        metaData.setType(StringUtils.substringBefore(textureInfo, "."));

        return true;
    }

    private String getType(int type) {
        switch (type) {
            case BufferedImage.TYPE_4BYTE_ABGR:
                return "4 Byte ABGR (6)";
            case BufferedImage.TYPE_3BYTE_BGR:
                return "4 Byte BGR (5)";
        }
        return "Unknown";
    }

    /**
     * Converts a 32 Bit RGBA Pixel Array into a GX Array
     *
     * @param data 32 Bit RGBA Pixel Array
     */
    private void convertTex32bpp(int[] data) {
        byte R, G, B, A;
        int color, x, y, ind = 0;
        int w = metaData.getWidth() - (metaData.getWidth() % 4);
        int h = metaData.getHeight() - (metaData.getHeight() % 4);
        short[] GX_buffer = new short[h * w * 2];

        for (int i = 0; i < h; i += 4) {
            for (int j = 0; j < w; j += 4) {
                for (int ii = 0; ii < 4; ii++) {
                    for (int jj = 0; jj < 4; jj++) {
                        x = j + jj;
                        y = i + ii;
                        color = data[(y * w) + x];
                        R = (byte) ((color >> 16) & 0xFF);
                        G = (byte) ((color >> 8) & 0xFF);
                        B = (byte) ((color) & 0xFF);
                        A = (byte) ((color >> 24) & 0xFF);
                        GX_buffer[ind] = (short) (((A & 0xFF) << 8) | (R & 0xFF));
                        GX_buffer[ind + 16] = (short) (((G & 0xFF) << 8) | (B & 0xFF));
                        ind++;
                    }
                }
                ind += 16;
            }
        }
        gxTexture = new byte[GX_buffer.length * 2];
        for (int i = 0; i < GX_buffer.length; i++) {
            gxTexture[i * 2] = (byte) (GX_buffer[i] >> 8);
            gxTexture[(i * 2) + 1] = (byte) (GX_buffer[i] & 0xFF);
        }
    }

    /**
     * Converts a 24 Bit RGB Pixel Array into a GX Array, either with alpha in a
     * separate buff or 0xFF or replacing it with B
     *
     * @param primaryPixels     the primary 24-bit rgb pixel array
     * @param alphaPixels       alpha pixels
     * @param replaceAlphaWithB are we replacing alpha with B?
     */
    private void convertTex24bpp(int[] primaryPixels, int[] alphaPixels, boolean replaceAlphaWithB) {
        byte R, G, B, A;
        int x, y, ind = 0;
        int w = metaData.getWidth() - (metaData.getWidth() % 4);
        int h = metaData.getHeight() - (metaData.getHeight() % 4);
        short[] GX_buffer = new short[h * w * 2];

        for (int i = 0; i < h; i += 4) {
            for (int j = 0; j < w; j += 4) {
                for (int ii = 0; ii < 4; ii++) {
                    for (int jj = 0; jj < 4; jj++) {
                        x = j + jj;
                        y = i + ii;
                        int color = primaryPixels[(y * w) + x];
                        R = (byte) ((color >> 16) & 0xFF);
                        G = (byte) ((color >> 8) & 0xFF);
                        B = (byte) ((color) & 0xFF);
                        if (alphaPixels != null) {
                            A = (byte) (alphaPixels[(y * w) + x] & 0xFF);
                        } else if (replaceAlphaWithB) {
                            A = B;
                        } else {
                            A = (byte) 0xFF;
                        }

                        GX_buffer[ind] = (short) (((A & 0xFF) << 8) | (R & 0xFF));
                        GX_buffer[ind + 16] = (short) (((G & 0xFF) << 8) | (B & 0xFF));
                        ind++;
                    }
                }
                ind += 16;
            }
        }
        gxTexture = new byte[GX_buffer.length * 2];
        for (int i = 0; i < GX_buffer.length; i++) {
            gxTexture[i * 2] = (byte) (GX_buffer[i] >> 8);
            gxTexture[(i * 2) + 1] = (byte) (GX_buffer[i] & 0xFF);
        }
    }

    byte[] getGXTexture() {
        return gxTexture;
    }

    ArchiveEntryMeta getMeta() {
        return metaData;
    }

}
