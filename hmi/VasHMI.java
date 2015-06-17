import javax.swing.*;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import java.awt.*;
import java.io.*;
import java.net.*;

public class VasHMI extends JPanel{
    private BufferedReader inFromUser;
    private Socket clientSocket;
    private DataOutputStream outToServer;
    private BufferedReader inFromServer;

    private JTextField field;
    private int id;
    private byte[] tcp_message = new byte[16];

    public VasHMI() {
        initializeUI();
    }

    private void initializeUI() {
        setLayout(new BorderLayout());
        setPreferredSize(new Dimension(600, 350));

	tcp_message[0] = (byte)0xff;

	inFromUser = new BufferedReader( new InputStreamReader(System.in));

	try{
		clientSocket = new Socket("localhost", 6789);
	} catch (IOException e) {
		System.err.println("Caught IOException: " + e.getMessage());
	}

	try{
		outToServer = new DataOutputStream(clientSocket.getOutputStream());
	}catch (IOException e) {
		System.err.println("Caught IOException: " + e.getMessage());
	}

	try{
		inFromServer = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
	}catch (IOException e) {
		System.err.println("Caught IOException: " + e.getMessage());
	}

        JSlider slider_brake = new JSlider(JSlider.VERTICAL, 0, 15, 0);

        slider_brake.setPaintTicks(true);
        slider_brake.setPaintLabels(true);
        slider_brake.setMinorTickSpacing(1);
        slider_brake.setMajorTickSpacing(5);

        JSlider slider_steer = new JSlider(JSlider.HORIZONTAL, 0, 127, 64);

        slider_steer.setPaintTicks(true);
        slider_steer.setPaintLabels(true);
        slider_steer.setMinorTickSpacing(1);
        slider_steer.setMajorTickSpacing(20);

        slider_steer.addChangeListener(new ChangeListener(){
		public void stateChanged(ChangeEvent e) {
			JSlider slider = (JSlider) e.getSource();
			tcp_message[1] = (byte)0x62;
			tcp_message[2] = (byte)(slider.getValue());

			try{
				outToServer.write(tcp_message, 0, 3);
			}catch (IOException ioe) {
				System.err.println("Caught IOException: " + ioe.getMessage());
			}
		}
	});

        slider_brake.addChangeListener(new ChangeListener(){
		public void stateChanged(ChangeEvent e) {
			JSlider slider = (JSlider) e.getSource();
			tcp_message[1] = (byte)0x63;
			tcp_message[2] = (byte)(slider.getValue());

			try{
				outToServer.write(tcp_message, 0, 3);
			}catch (IOException ioe) {
				System.err.println("Caught IOException: " + ioe.getMessage());
			}
		}
	});

        JSlider slider_accel = new JSlider(JSlider.VERTICAL, 0, 15, 0);

        slider_accel.setPaintTicks(true);
        slider_accel.setPaintLabels(true);
        slider_accel.setMinorTickSpacing(1);
        slider_accel.setMajorTickSpacing(5);

        slider_accel.addChangeListener(new ChangeListener(){
		public void stateChanged(ChangeEvent e) {
			JSlider slider = (JSlider) e.getSource();
			tcp_message[1] = (byte)0x64;
			tcp_message[2] = (byte)(slider.getValue());

			try{
				outToServer.write(tcp_message, 0, 3);
			}catch (IOException ioe) {
				System.err.println("Caught IOException: " + ioe.getMessage());
			}
		}
	});



        add(slider_brake, BorderLayout.WEST);
        add(slider_accel, BorderLayout.EAST);
        add(slider_steer, BorderLayout.CENTER);

    }

    public static void showFrame() {
        JPanel panel = new VasHMI();
        panel.setOpaque(true);

        JFrame frame = new JFrame();
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setTitle("JSlider Demo");
        frame.setContentPane(panel);
        frame.pack();
        frame.setVisible(true);
    }

    public static void main(String[] args) {
		SwingUtilities.invokeLater(new Runnable() {
		    public void run() {
			VasHMI.showFrame();
		    }
		});
    }
}
