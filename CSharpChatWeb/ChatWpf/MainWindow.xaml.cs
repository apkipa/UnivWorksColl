using ChatWpf.Misc;
using Flurl.Http;
using Flurl.Util;
using HandyControl.Controls;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ChatWpf
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : System.Windows.Window
    {
        public MainWindow()
        {
            InitializeComponent();

            Dispatcher.UnhandledException += (sender, e) =>
            {
                if (e.Exception is FlurlHttpException fe)
                {
                    var resp = fe.GetResponseStringAsync().Result;
                    var isJson = fe.Call.Response.Headers.GetAll("Content-Type").FirstOrDefault()?.Contains("application/json") ?? false;
                    if (isJson)
                    {
                        JsonSerializerOptions jsonSerializerOptions = new JsonSerializerOptions
                        {
                            Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping,
                        };
                        Utils.GrowlError($"HTTP {fe.StatusCode}: {JsonSerializer.Serialize(JsonObject.Parse(resp), jsonSerializerOptions)}");
                    }
                    else
                    {
                        Utils.GrowlError($"HTTP {fe.StatusCode}: {resp}");
                    }
                }
                else
                {
                    Utils.GrowlError(e.Exception.Message);
                    //Growl.Error(e.Exception.Message);
                }
                e.Handled = true;
            };

            TaskScheduler.UnobservedTaskException += (sender, e) =>
            {
                Utils.GrowlError(e.Exception.Message);
                e.SetObserved();
            };

            RootFrame.Navigate(new Pages.LoginPage());
        }
    }
}