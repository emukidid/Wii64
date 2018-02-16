package com.wii64.texpacker.packer;

/**
 * Contains Meta data for a archive entry
 *
 * @author emu_kidid
 */
public class ArchiveEntryMeta {
    private String type, fileName;
    private Long textureCRC, paletteCRC, offset;
    private Short format, size;
    private Integer compressedLength, width, height;
    private Integer alphaWidth, alphaHeight;
    private String rawPixelType, rawAlphaPixelType;
    private String errorMsg;

    ArchiveEntryMeta() {
    }

    void setTextureCRC(Long textureCRC) {
        this.textureCRC = textureCRC;
    }

    void setPaletteCRC(Long paletteCRC) {
        this.paletteCRC = paletteCRC;
    }

    void setFormat(Short format) {
        this.format = format;
    }

    void setSize(Short size) {
        this.size = size;
    }

    void setCompressedLength(Integer compressedLength) {
        this.compressedLength = compressedLength;
    }

    Integer getWidth() {
        return width;
    }

    void setWidth(Integer width) {
        this.width = width;
    }

    Integer getHeight() {
        return height;
    }

    void setHeight(Integer height) {
        this.height = height;
    }

    Long getOffset() {
        return offset;
    }

    void setOffset(Long offset) {
        this.offset = offset;
    }

    void setType(String type) {
        this.type = type;
    }

    String getFileName() {
        return fileName;
    }

    void setFileName(String fileName) {
        this.fileName = fileName;
    }

    public Long getTextureCRC() {
        return textureCRC;
    }

    Long getCRC64() {
        // unsigned int crc32;
        // unsigned char pal_crc32_byte1;
        // unsigned char pal_crc32_byte2;
        // unsigned char pal_crc32_byte3;
        // unsigned format : 4;
        // unsigned size : 4;
        return (((textureCRC) << 32) | (paletteCRC & 0x00000000FFFFFF00L) | ((format & 0xF) << 4) | (size & 0xF));
    }

    public String toString() {
        return fileName + " textureCRC: " + Long.toHexString(textureCRC).toUpperCase() + "(type: " + type
                + ") paletteCRC: " + Long.toHexString(paletteCRC).toUpperCase() + "\nformat: "
                + Integer.toHexString(format).toUpperCase() + " size: " + Integer.toHexString(size).toUpperCase()
                + " width * height: " + width + " x " + height + " compressedLength: " + compressedLength
                + "\nu64 crc: " + Long.toHexString(getCRC64()).toUpperCase() + " Pixel Data Type: " + rawPixelType
                + (rawAlphaPixelType != null ? (" Alpha Type: " + rawAlphaPixelType) : "");
    }

    Short getGXFormat() {
        return 6; // GX_TF_RGBA8
    }

    Integer getAlphaWidth() {
        return alphaWidth;
    }

    void setAlphaWidth(Integer alphaWidth) {
        this.alphaWidth = alphaWidth;
    }

    Integer getAlphaHeight() {
        return alphaHeight;
    }

    void setAlphaHeight(Integer alphaHeight) {
        this.alphaHeight = alphaHeight;
    }

    void setRawPixelType(String rawPixelType) {
        this.rawPixelType = rawPixelType;
    }

    void setRawAlphaPixelType(String rawAlphaPixelType) {
        this.rawAlphaPixelType = rawAlphaPixelType;
    }

    String getErrorMsg() {
        return this.errorMsg;
    }

    void setErrorMsg(String errorMsg) {
        this.errorMsg = errorMsg;
    }
}
