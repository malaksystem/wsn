/**
 * RSSI Viewer - view RSSI values of 802.15.4 channels
 * ---------------------------------------------------
 * Note: run the rssi-scanner on a Sky or sentilla node connected to USB
 * then start with
 * make ViewRSSI.class
 * make login | java ViewRSSI
 * or
 * make viewrssi
 * 
 * Created: Fri Apr 24 00:40:01 2009, Joakim Eriksson
 *
 * @author Joakim Eriksson, SICS, Easinet
 * @version 1.0
 */
import javax.swing.*;
import java.awt.*;
import java.io.*;
import java.io.File;  

public class ViewRSSI extends JPanel {
	

	private static File file;  

	private int[] rssi = new int[80];
	private int[] rssiMax = new int[80];
	/* 55 is added by the scanner. 45 is the offset of the CC2420 */
	private final int DELTA = -55 -45; 
	
	/* this is the max value of the RSSI from the cc2420 */
	private static final int RSSI_MAX_VALUE = 200;
	
	public ViewRSSI() {
	}
	
	public void paint(Graphics g) {
		
		int h = getHeight();
		int w = getWidth();
		double factor = (h - 20.0) / RSSI_MAX_VALUE;
		double sSpacing = (w - 15) / 80.0;
		int sWidth = (int) (sSpacing - 1);
		if (sWidth == 0)
			sWidth = 1;
		
		Graphics2D g2d=(Graphics2D)g;
		Font myFont=new Font("Arial", Font.PLAIN, 8);		
		g2d.setFont( myFont ); 
		
		g.setColor(Color.white);
		g.fillRect(0, 0, w, h);
		
		g.setColor(Color.gray);
		double xpos = 10;
		for (int i = 0, n = rssi.length; i < n; i++) {
			int rssi = (int) (rssiMax[i] * factor);
			g.fillRect((int) xpos, h - 20 - rssi, sWidth, rssi + 1);
			g2d.drawString(Integer.toString(rssiMax[i] + DELTA), (int)xpos,h - 20 - rssi - 5 );
			xpos += sSpacing;
		}
		
		
		xpos = 10;
		for (int i = 0, n = rssi.length; i < n; i++) {
			g.setColor(Color.black);
			int rssiVal = (int) (rssi[i] * factor);
			g.fillRect((int) xpos, h - 20 - rssiVal, sWidth, rssiVal + 1);
			g2d.drawString(Integer.toString(rssi[i] + DELTA), (int)xpos,h - 20 - rssiVal - 8 );
			g.setColor(Color.white);
			g2d.drawString(Float.toString((float)i / 5 + (float)56/5), (int)xpos,h - 20 - 5 );
			xpos += sSpacing;
		}
	}
	
	private void handleInput() throws IOException {
		BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
		FileOutputStream out=new FileOutputStream(file,true);      
    PrintStream p=new PrintStream(out);
		while (true) {
			String line = reader.readLine();
			if (line.startsWith("RSSI:")) {
				try {
					String[] parts = line.substring(5).split(" ");
					for (int i = 0, n = parts.length; i < n; i++) {
						rssi[i] = 3 * Integer.parseInt(parts[i]);

					if(i > 73 && i < 77)		//specific ch
					{
						p.print(rssi[i]/3 + DELTA);		//raw P in dB
            p.print(' ');
					}
           


						if (rssi[i] >= rssiMax[i])
							rssiMax[i] = rssi[i];
						else if (rssiMax[i] > 0)
							rssiMax[i]--;
					}
				} catch (Exception e) {
					/* report but do not fail... */
					e.printStackTrace();
				}
				repaint();
			}
			p.print("\n");
		}
	}
	
	public static void main(String[] args) throws IOException {
		JFrame win = new JFrame("RSSI Viewer");
		ViewRSSI panel;
		win.setBounds(10, 10, 300, 300);
		win.getContentPane().add(panel = new ViewRSSI());
		win.setVisible(true);
		win.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		file = new File("/home/wsn/plot/rssi.txt");
		if(!file.exists())
             file.createNewFile();



		System.out.println("test");
		
		panel.handleInput();
	}
}
