using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace amule.net
{
    public partial class MainWindow : Form
    {
        amuleRemote amuleRemote = new amuleRemote();

        public MainWindow()
        {
            InitializeComponent();
        }

        private void MainWindow_Load(object sender, EventArgs e)
        {
            ConnectDlg connectDlg = new ConnectDlg();
            string errorMsg = null;
            bool connect_ok = false;
            while ( !connect_ok ) {
                if (connectDlg.ShowDialog() == DialogResult.OK) {
                    connect_ok = amuleRemote.ConnectToCore("leox", 9999, "123456", ref errorMsg);
                    if (!connect_ok) {
                        Console.WriteLine("Connect failed '{0}'", errorMsg);
                    }
                } else {
                    Application.Exit();
                    return;
                }
            }
        }
    }
}