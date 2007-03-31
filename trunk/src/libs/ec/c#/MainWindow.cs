using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace amule.net
{
    public class aMuleStats {
        int bpsUp, bpsDown;
        UInt32 ID;
        string server;
    }

    public partial class MainWindow : Form
    {
        amuleRemote amuleRemote = new amuleRemote();
        Timer updateTimer = null;

        public MainWindow()
        {
            InitializeComponent();
        }

        private static void UpdateTimerProc(Object myObject,
                                            EventArgs myEventArgs)
        {
            MainWindow w = (MainWindow)((Timer)myObject).Tag;
        }

        private void MainWindow_Load(object sender, EventArgs e)
        {
            //
            // First, attempt to connect to remote core
            //
            ConnectDlg connectDlg = new ConnectDlg();
            string errorMsg = null;
            bool connect_ok = false;
            string amuleHost = "", amulePort = "";
            while ( !connect_ok ) {
                if (connectDlg.ShowDialog() == DialogResult.OK) {
                    amuleHost = connectDlg.Host();
                    amulePort = connectDlg.Port();
                    string pass = connectDlg.Pass();
                    int amulePort_int = 0;
                    try {
                        amulePort_int = Convert.ToInt16(amulePort, 10);
                    }catch(Exception) {
                        MessageBox.Show("Invalid port number", "Error",
                            MessageBoxButtons.OK, MessageBoxIcon.Error);
                        continue;
                    }
                    connect_ok = amuleRemote.ConnectToCore(amuleHost, amulePort_int, pass, ref errorMsg);
                    if (!connect_ok) {
                        Console.WriteLine("Connect failed '{0}'", errorMsg);
                    }
                } else {
                    Application.Exit();
                    return;
                }
            }
            statusStrip.Items["toolStripConnectionStatus"].Text =
                "Connected to [" + amuleHost + ":" + amulePort + "]";
            //
            // Connection OK at this point
            //
            updateTimer = new Timer();
            updateTimer.Tag = this;
            updateTimer.Tick += new EventHandler(UpdateTimerProc);
            updateTimer.Interval = 1000;
            updateTimer.Start();
        }
    }
}