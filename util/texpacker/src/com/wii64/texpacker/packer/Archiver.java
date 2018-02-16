package com.wii64.texpacker.packer;

import com.wii64.texpacker.component.RiverLayout;
import com.wii64.texpacker.util.Log;
import org.apache.commons.lang.StringUtils;

import javax.imageio.ImageIO;
import javax.swing.*;
import javax.swing.text.AttributeSet;
import javax.swing.text.BadLocationException;
import javax.swing.text.PlainDocument;
import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.image.BufferedImage;
import java.io.*;
import java.math.BigInteger;
import java.sql.Date;
import java.text.SimpleDateFormat;
import java.util.AbstractMap.SimpleEntry;
import java.util.*;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * The actual archive
 * <p>
 * Header: u8 magic[4]; //'GXA1'<br>
 * u8 description[64]; // Pak description<br>
 * u8 author[16]; // Pak original author<br>
 * u8 com.wii64.texpacker.packer[16]; // TexPacker of this archive<br>
 * u8 datepacked[12]; // Date packed yyyy/mm/dd<br>
 * u8 icon[96*72*2]; // RGB5A3 icon<br>
 * u32 table_location; // LUT location in the pack <br>
 * u32 num_entries; // Number of entries in the LUT
 * <p>
 * LUT Entry: <br>
 * u64 custom structure made up of the following: <br>
 * unsigned int crc32; <br>
 * unsigned char pal_crc32_byte1; <br>
 * unsigned char pal_crc32_byte2; <br>
 * unsigned char pal_crc32_byte3; <br>
 * unsigned format : 4; <br>
 * unsigned size : 4; <br>
 * u32 offset;
 * <p>
 * Compressed Entry: <br>
 * u32 width; <br>
 * u32 height; <br>
 * u8 gxtex[compressedSize];
 *
 * @author emu_kidid
 */
@SuppressWarnings("serial")
class Archiver extends JFrame {

    private String version = "v1.2";
    private boolean startPushed = false;
    private RandomAccessFile fw;
    private List<File> textureFiles;
    /* GUI crap */
    private boolean pakSelected = false;
    private boolean dirSelected = false;
    private JLabel jProgLabel;
    private Archiver mainFrame;
    // List of crc64 and offset
    private List<SimpleEntry<Long, Long>> crcLookupTable;
    private int numAdded = 0;
    private String texPath = null;
    private String pakPath = null;
    private String icoPath = null;
    private JButton startBtn = new JButton("Start!");
    private JButton dirBtn = new JButton("...");
    private JButton pakBtn = new JButton("...");
    private JButton icoBtn = new JButton("...");
    private JButton packInfoBtn = new JButton("Pack Info...");
    private JTextField jtxAuthor = new JTextField();
    private JTextField jtxPacker = new JTextField();
    private JTextField jtxDescription = new JTextField();
    private JLabel lblBannerIco = new JLabel();
    private BufferedImage img = null;
    private long tableLocPtr = 0;
    // Singleton
    private static Archiver instance;

    private Archiver() {
        this.mainFrame = this;
        final JLabel dirTxt = new JLabel("Select a Directory containing extracted N64 hi-res textures ...");
        final JLabel pakTxt = new JLabel("Select a File to output the texture archive to ...");
        final JLabel icoTxt = new JLabel("Select a 96x72 image file to use a pak banner ...");

        this.setTitle("Rice Tex TexPacker for Wii64 " + version + " by emu_kidid");
        this.setMinimumSize(new Dimension(640, 420));
        this.setPreferredSize(new Dimension(640, 420));
        this.setResizable(false);
        this.addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                System.exit(0);
            }
        });

        JLabel lblWarn = new JLabel();
        jtxAuthor.setDocument(new LimitedSizeDocument(16, lblWarn, jtxAuthor));
        jtxAuthor.setPreferredSize(new Dimension(450, 21));
        jtxPacker.setPreferredSize(new Dimension(450, 21));
        jtxPacker.setDocument(new LimitedSizeDocument(16, lblWarn, jtxPacker));
        jtxDescription.setPreferredSize(new Dimension(450, 21));
        jtxDescription.setDocument(new LimitedSizeDocument(64, lblWarn, jtxDescription));

        dirBtn.setPreferredSize(new Dimension(25, 21));
        pakBtn.setPreferredSize(new Dimension(25, 21));
        icoBtn.setPreferredSize(new Dimension(25, 21));
        lblBannerIco.setPreferredSize(new Dimension(96, 72));
        lblBannerIco.setBorder(BorderFactory.createLoweredBevelBorder());

        dirTxt.setPreferredSize(new Dimension(450, 21));
        dirTxt.setBorder(BorderFactory.createEtchedBorder());
        pakTxt.setPreferredSize(new Dimension(450, 21));
        pakTxt.setBorder(BorderFactory.createEtchedBorder());
        icoTxt.setPreferredSize(new Dimension(450, 21));
        icoTxt.setBorder(BorderFactory.createEtchedBorder());

        jProgLabel = new JLabel();
        jProgLabel.setHorizontalTextPosition(SwingConstants.CENTER);
        Container c = this.getContentPane();
        c.setLayout(new RiverLayout());

        c.add("center", new JLabel("Rice Tex TexPacker for Wii64 " + version + " by emu_kidid, sepp256 & tehpola"));
        c.add("p left", new JLabel("Tex Input:"));
        c.add("tab", dirTxt);
        c.add("tab", dirBtn);
        c.add("br", new JLabel("Pak Output:"));
        c.add("tab", pakTxt);
        c.add("tab", pakBtn);
        c.add("br", lblWarn);
        c.add("br", new JLabel("Author:"));
        c.add("tab", jtxAuthor);
        c.add("br", new JLabel("TexPacker:"));
        c.add("tab", jtxPacker);
        c.add("br", new JLabel("Description:"));
        c.add("tab", jtxDescription);
        c.add("br", new JLabel("Icon Banner:"));
        c.add("tab", icoTxt);
        c.add("tab", icoBtn);
        c.add("br", new JLabel("Image Preview:"));
        c.add("tab", lblBannerIco);
        c.add("p centre hfill", jProgLabel);
        c.add("p center", startBtn);
        c.add("p right", packInfoBtn);

        startBtn.setEnabled(false);
        startBtn.addActionListener(arg0 -> setStartPushed(true));
        pakBtn.addActionListener(e -> {
            FileDialog fileDialog = new FileDialog(Archiver.this, "Select an output filename", FileDialog.SAVE);
            fileDialog.setFile("*.pak");
            fileDialog.setFilenameFilter((dir, name) -> name.endsWith(".pak"));
            fileDialog.setVisible(true);
            if (fileDialog.getFile() != null) {
                pakTxt.setText(fileDialog.getFiles()[0].getAbsolutePath());
                pakPath = pakTxt.getText();
                pakSelected = true;
                startBtn.setEnabled(dirSelected);
            }
        });
        dirBtn.addActionListener(e -> {
            FileDialog fileDialog = new FileDialog(Archiver.this, "Select a texture pack zip archive", FileDialog.LOAD);
            fileDialog.setFile("*.zip");
            fileDialog.setFilenameFilter((dir, name) -> name.endsWith(".zip"));
            fileDialog.setVisible(true);
            if (fileDialog.getDirectory() != null) {
                dirTxt.setText(fileDialog.getFiles()[0].getAbsolutePath());
                texPath = dirTxt.getText();
                dirSelected = true;
                startBtn.setEnabled(pakSelected);
            }
        });
        icoBtn.addActionListener(e -> {
            FileDialog fileDialog = new FileDialog(Archiver.this, "Select an pak image (96x72 max)", FileDialog.LOAD);
            fileDialog.setVisible(true);
            if (fileDialog.getFile() != null) {
                icoTxt.setText(fileDialog.getFiles()[0].getAbsolutePath());
                icoPath = icoTxt.getText();
                new Thread(this::checkAndLoadImage).run();
            }
        });
        packInfoBtn.addActionListener(arg0 -> {
            JDialog dlg = new JDialog();
            // @formatter:off
            JTextArea header = new JTextArea(
                    "u8 magic[4]; //'GXA1'\n"
                            + "u8 description[64]; // Pak description\n"
                            + "u8 author[16]; // Pak original author\n"
                            + "u8 com.wii64.texpacker.packer[16]; // TexPacker of this archive\n"
                            + "u8 datepacked[12]; // Date packed yyyy/mm/dd\n"
                            + "u8 icon[96*72*2]; // RGB5A3 icon\n"
                            + "u32 table_location; // LUT location in the pack\n"
                            + "u32 num_entries; // Number of entries in the file");
            JTextArea compressedEntry = new JTextArea(
                    "u64 crc structure made up of the following:\n"
                            + "unsigned int  crc32;\n"
                            + "unsigned char pal_crc32_byte1;\n"
                            + "unsigned char pal_crc32_byte2;\n"
                            + "unsigned char pal_crc32_byte3;\n"
                            + "unsigned format : 4;\n"
                            + "unsigned size   : 4;\n"
                            + "u32 offset");
            JTextArea lutEntry = new JTextArea(
                    "u8 width div 4;\n"
                            + "u8 height div 4;\n"
                            + "u8 gx_fmt;\n"
                            + "u8 gxTex[compressedSize]");
            // @formatter:on
            header.setEditable(false);
            compressedEntry.setEditable(false);
            lutEntry.setEditable(false);
            dlg.setModal(true);
            dlg.setMinimumSize(new Dimension(750, 450));
            dlg.setPreferredSize(new Dimension(750, 450));
            dlg.setLayout(new RiverLayout());
            dlg.add("center", new JLabel("Rice Tex TexPacker Pack Info"));
            dlg.add("p left", new JLabel("Header:"));
            dlg.add("tab hfill", header);
            dlg.add("p left", new JLabel("Compressed Entry:"));
            dlg.add("tab hfill", compressedEntry);
            dlg.add("p left", new JLabel("LUT Entry:"));
            dlg.add("tab hfill", lutEntry);
            dlg.setVisible(true);
        });
    }

    static Archiver getInstance() {
        if (instance == null) {
            instance = new Archiver();
        }
        return instance;
    }

    private void checkAndLoadImage() {
        if (icoPath != null) {
            File iconFile = new File(icoPath);
            if (iconFile.isFile() && iconFile.canRead()) {
                try {
                    img = ImageIO.read(iconFile);
                    if (img.getWidth() != 96 || img.getHeight() != 72) {
                        JOptionPane.showMessageDialog(mainFrame, "Image must be 96x72, this image will not be used.");
                        icoPath = "";
                        img = null;
                    } else {
                        lblBannerIco.setIcon(new ImageIcon(img));
                    }
                } catch (IOException e) {
                    JOptionPane.showMessageDialog(mainFrame, "Error reading banner icon image!");
                    icoPath = "";
                    img = null;
                }
            }
        }
    }

    private void toggleComponents(boolean editable) {
        startBtn.setEnabled(editable);
        packInfoBtn.setEnabled(editable);
        dirBtn.setEnabled(editable);
        pakBtn.setEnabled(editable);
        icoBtn.setEnabled(editable);
        jtxAuthor.setEditable(editable);
        jtxAuthor.setEnabled(editable);
        jtxPacker.setEditable(editable);
        jtxPacker.setEnabled(editable);
        jtxDescription.setEditable(editable);
        jtxDescription.setEnabled(editable);
    }

    void run() {
        Thread t = new Thread("TexPacker Thread") {
            @Override
            public void run() {
                SwingUtilities.invokeLater(() -> toggleComponents(false));

                File outputArchive = new File(getPakPath());
                try {
                    numAdded = 0;
                    crcLookupTable = new ArrayList<>();
                    textureFiles = new ArrayList<>();
                    fw = new RandomAccessFile(outputArchive, "rw");
                    initializePak();
                    try (ZipInputStream zipInputStream = new ZipInputStream(new FileInputStream(getTexturePath()))) {
                        ZipEntry zipEntry;
                        while ((zipEntry = zipInputStream.getNextEntry()) != null) {
                            getTextureFiles(zipInputStream, zipEntry);
                        }
                    }
                    if (processTextures()) {
                        finalizePak();
                        printStats();
                        JOptionPane.showMessageDialog(mainFrame, "Successfully created texture pak!");
                    } else {
                        JOptionPane.showMessageDialog(mainFrame, "Failed to create texture pak!");
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                    JOptionPane.showMessageDialog(mainFrame, "Failed to create texture pak!", "Texpacker", JOptionPane.ERROR_MESSAGE);
                }
                SwingUtilities.invokeLater(() -> toggleComponents(true));
            }
        };
        t.run();

    }

    /**
     * Performs texture file priority based on type (_all,_rgb,_a, etc) and
     * processes/adds files
     */
    private boolean processTextures() {
        // Don't keep redundant files based on a hierarchy
        List<File> tmp = new ArrayList<>();
        for (File textureFile : textureFiles) {
            String curFileName = textureFile.getName();
            if (StringUtils.endsWithIgnoreCase(curFileName, "_all.png")) {
                // If we have an _all.png file, we want it
                tmp.add(textureFile);
            }
        }
        // Look to see if we have any _rgb.png or _a.png files, we want these
        // only if the _all.png doesn't exist with the same name
        for (File textureFile : textureFiles) {
            String curFileName = textureFile.getName();
            if (StringUtils.endsWithIgnoreCase(curFileName, "_rgb.png") || StringUtils.endsWithIgnoreCase(curFileName, "_a.png")) {
                boolean found = false;
                for (File aTmp : tmp) {
                    if (StringUtils.endsWithIgnoreCase(aTmp.getName(), "_all.png")) {
                        String s1 = StringUtils.substringBeforeLast(curFileName, "_");
                        String s2 = StringUtils.substringBeforeLast(aTmp.getName(), "_");
                        if (StringUtils.equalsIgnoreCase(s1, s2)) {
                            found = true;
                            break;
                        }
                    }
                }
                if (!found) {
                    tmp.add(textureFile);
                }
            }
        }
        // TODO: just keep _ciByRGBA.png type for now ??
        for (File textureFile : textureFiles) {
            if (StringUtils.endsWithIgnoreCase(textureFile.getName(), "_ciByRGBA.png")) {
                tmp.add(textureFile);
            }
        }
        textureFiles.clear();
        textureFiles.addAll(tmp);
        tmp.clear();

        // Add them all now
        for (int i = 0; i < textureFiles.size(); i++) {
            File primaryFile = textureFiles.get(i);
            File alphaFile = null;
            // Pair alpha with _rgb files (if they exist)
            if (!StringUtils.endsWithIgnoreCase(primaryFile.getName(), "_a.png")) {
                if (StringUtils.endsWithIgnoreCase(primaryFile.getName(), "_rgb.png")) {
                    for (File textureFile : textureFiles) {
                        if (StringUtils.endsWithIgnoreCase(textureFile.getName(), "_a.png")) {
                            String s1 = StringUtils.substringBeforeLast(primaryFile.getName(), "_") + "_a.png";
                            String s2 = textureFile.getName();
                            if (StringUtils.equalsIgnoreCase(s1, s2)) {
                                alphaFile = textureFile;
                            }
                        }
                    }
                }
                ArchiveEntry newEntry = new ArchiveEntry(primaryFile, alphaFile);
                if (newEntry.process()) {
                    addEntry(newEntry);
                } else {
                    Log.info("\r\nERROR: \r\n" + newEntry.getMeta().getFileName() + "\r\nFailed to process due to: "
                            + newEntry.getMeta().getErrorMsg());
                    return false;
                }
                jProgLabel.setText(i + 1 + " of " + textureFiles.size() + " processed");
            }
        }
        return true;
    }

    private File extractFile(ZipInputStream zipInputStream, ZipEntry zipEntry) throws IOException {
        byte[] buffer = new byte[1024];

        File newFile = new File(System.getProperty("java.io.tmpdir") + File.separator + "texpacker" + File.separator + zipEntry.getName());
        File parentDir = new File(newFile.getParent());
        parentDir.mkdirs();
        parentDir.deleteOnExit();

        try (FileOutputStream fos = new FileOutputStream(newFile)) {
            int len;
            while ((len = zipInputStream.read(buffer)) > 0) {
                fos.write(buffer, 0, len);
            }
        }
        newFile.deleteOnExit();
        return newFile;
    }

    /**
     * Gets all the files which might be textures
     */
    private void getTextureFiles(ZipInputStream zipInputStream, ZipEntry zipEntry) throws IOException {
        if (!zipEntry.isDirectory() && StringUtils.endsWith(zipEntry.getName(), ".png") && StringUtils.contains(zipEntry.getName(), "#")
                && zipEntry.getSize() > 0) {
            textureFiles.add(extractFile(zipInputStream, zipEntry));
            jProgLabel.setText(textureFiles.size() + " possible textures found");
            Log.info("Number of image files found: " + textureFiles.size());
        }
    }

    /**
     * Initialize the pak by writing the header and skipping to 0x20
     */
    private void initializePak() throws IOException {
        String magic = "GXA1";
        fw.write(magic.getBytes());
        fw.write(Arrays.copyOf(jtxDescription.getText().getBytes(), 64));
        fw.write(Arrays.copyOf(jtxAuthor.getText().getBytes(), 16));
        fw.write(Arrays.copyOf(jtxPacker.getText().getBytes(), 16));
        SimpleDateFormat format = new SimpleDateFormat("yyyy/MM/dd");
        String date = format.format(new Date(System.currentTimeMillis()));
        fw.write(Arrays.copyOf(date.getBytes(), 12));
        if (img != null) {
            fw.write(convertPakIconToRGB5A3(img));
        } else {
            fw.write(Arrays.copyOf(new byte[]{0}, 96 * 72 * 2));
        }
        tableLocPtr = fw.getFilePointer();
        fw.seek(tableLocPtr + 8); // Leave room for LUT location + number
        // of entries in the file
    }

    /**
     * Sort the CRC table by lowest to highest CRC, update the header and
     * finally write the table
     */
    private void finalizePak() throws IOException {
        crcLookupTable.sort(Comparator.comparing(o -> (new BigInteger(1, longToBytes(o.getKey())))));
        // 4 byte align the TOC
        if ((fw.getFilePointer() % 4) != 0) {
            fw.write(Arrays.copyOf(new byte[]{0}, (int) (4 - (fw.getFilePointer() % 4))));
        }
        long tableLoc = fw.getFilePointer();
        fw.seek(tableLocPtr);
        fw.writeInt((int) tableLoc);
        fw.writeInt(crcLookupTable.size());
        fw.seek(tableLoc);
        for (Map.Entry<Long, Long> entry : crcLookupTable) {
            fw.writeLong(entry.getKey());
            fw.writeInt(entry.getValue().intValue());
        }
        fw.close();
    }

    /**
     * Adds an entry to the zipped stream and updates the LUT
     */
    private void addEntry(ArchiveEntry entry) {
        try {
            // 4 byte align the file entry
            if ((fw.getFilePointer() % 4) != 0) {
                fw.write(Arrays.copyOf(new byte[]{0}, (int) (4 - (fw.getFilePointer() % 4))));
            }
            // Update the Entry Meta with our current File Pointer
            entry.getMeta().setOffset(fw.getFilePointer());

            // Add it to our LUT
            crcLookupTable.add(new SimpleEntry<>(entry.getMeta().getCRC64(), entry.getMeta().getOffset()));

            // Write out the Actual Entry (Width+Height+GXTexture)
            fw.writeByte(new Integer(entry.getMeta().getWidth() / 4).byteValue());
            fw.writeByte(new Integer(entry.getMeta().getHeight() / 4).byteValue());
            fw.writeByte(entry.getMeta().getGXFormat().byteValue());
            fw.write(entry.getGXTexture());

            // Update the log window
            Log.info(entry.getMeta().toString());
            numAdded++;
        } catch (IOException e) {
            e.printStackTrace();
            Log.info("Failed to add entry to pak! " + entry.getMeta().toString());
        }
    }

    private void printStats() {
        Log.info("Entries added: " + numAdded);
    }

    /**
     * @return The root path of all of the individual .png files
     */
    private String getTexturePath() {
        return texPath;
    }

    /**
     * @return The path of the compressed and final pak path
     */
    private String getPakPath() {
        return pakPath;
    }

    private byte[] longToBytes(long v) {
        byte[] writeBuffer = new byte[8];

        writeBuffer[0] = (byte) (v >>> 56);
        writeBuffer[1] = (byte) (v >>> 48);
        writeBuffer[2] = (byte) (v >>> 40);
        writeBuffer[3] = (byte) (v >>> 32);
        writeBuffer[4] = (byte) (v >>> 24);
        writeBuffer[5] = (byte) (v >>> 16);
        writeBuffer[6] = (byte) (v >>> 8);
        writeBuffer[7] = (byte) (v);

        return writeBuffer;
    }

    boolean isStartPushed() {
        return startPushed;
    }

    void setStartPushed(boolean startPushed) {
        this.startPushed = startPushed;
    }

    private class LimitedSizeDocument extends PlainDocument {
        int maxChars;
        JLabel lblWarning;
        JTextField jtxComponent;

        LimitedSizeDocument(int maxChars, JLabel lblWarning, JTextField jtxComponent) {
            this.maxChars = maxChars;
            this.lblWarning = lblWarning;
            this.jtxComponent = jtxComponent;
            jtxComponent.setBorder(BorderFactory.createEtchedBorder());
        }

        public void insertString(int offset, String str, AttributeSet attr) throws BadLocationException {
            if (str == null)
                return;

            if ((getLength() + str.length()) <= maxChars) {
                super.insertString(offset, str, attr);
                lblWarning.setText("");
                jtxComponent.setBorder(BorderFactory.createEtchedBorder());
            } else {
                lblWarning.setText("The highlighted field has a " + maxChars + " char limit");
                jtxComponent.setBorder(BorderFactory.createLineBorder(Color.RED));
            }
        }

    }

    /**
     * RGB5A3
     * <p>
     * The RGB5A3 format is used for storing either 15 bit color values without
     * alpha, or 12 bit color values with a 3 bit alpha channel. The top bit is
     * used to decide between the two. If the top bit is set, the alpha channel
     * is used.
     * <p>
     * Conversion to RGBA is achieved by checking the top bit, and if it is not
     * set, setting A to 0x20 multiplied by the next 3 bits, and then setting R,
     * G and B in that order to the next 4 bits multiplied by 0x11 in each case.
     * If the top bit is set, the conversion is done by setting R, G and B in
     * that order to the next 5 bits multiplied by 0x8 in each case.
     *
     * @return rgb5a3 array
     */
    private byte[] convertPakIconToRGB5A3(BufferedImage img) {
        int data[];
        int h = img.getHeight(), w = img.getWidth(), x, y, color, idx = 0;
        byte R, G, B, A;
        short[] RGB5A3_buffer = new short[h * w];

        data = img.getRGB(0, 0, w, h, null, 0, w);
        for (int i = 0; i < h; i += 4) {
            for (int j = 0; j < w; j += 4) {
                for (int ii = 0; ii < 4; ii++) {
                    for (int jj = 0; jj < 4; jj++) {
                        x = j + jj;
                        y = i + ii;
                        color = data[((y * w) + x)];
                        R = (byte) ((color >> 16) & 0xFF);
                        G = (byte) ((color >> 8) & 0xFF);
                        B = (byte) ((color) & 0xFF);
                        A = (byte) ((color >> 24) & 0xFF);
                        if ((A & 0xE0) != 0xE0) {
                            RGB5A3_buffer[idx] = (short) (((A >> 5) << 12) | ((R >> 4) << 8) | ((G >> 4) << 4) | (B >> 4));
                        } else {
                            RGB5A3_buffer[idx] = (short) (0x8000 | ((R >> 3) << 10) | ((G >> 3) << 5) | (B >> 3));
                        }
                        idx++;
                    }
                }
            }
        }

        byte[] gxTexture = new byte[RGB5A3_buffer.length * 2];
        for (int i = 0; i < RGB5A3_buffer.length; i++) {
            gxTexture[i * 2] = (byte) (RGB5A3_buffer[i] >> 8);
            gxTexture[(i * 2) + 1] = (byte) (RGB5A3_buffer[i] & 0xFF);
        }
        return gxTexture;
    }

}
