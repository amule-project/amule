using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace amule.net
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            amuleRemote amuleRemote = new amuleRemote();

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainWindow());
            //Application.Run(new ConnectDlg());
            //ConnectDlg connectDlg = new ConnectDlg();
            //string errorMsg = null;
            //if (connectDlg.ShowDialog() == DialogResult.OK) {
            //    if ( !amuleRemote.ConnectToCore("leox", 9999, "123456", ref errorMsg) ) {
            //        Console.WriteLine("Connect failed '{0}'", errorMsg);
            //    }
            //} else {
            //    Application.Exit();
            //}
        }
    }
}