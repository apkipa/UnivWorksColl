using ChatWpf.Misc;
using HandyControl.Controls;
using HandyControl.Data;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ChatWpf.Pages
{
    /// <summary>
    /// Interaction logic for LoginPage.xaml
    /// </summary>
    public partial class LoginPage : Page
    {
        private const string LoginFailureMessage = "用户名或密码错误。";

        public LoginPage()
        {
            InitializeComponent();

            // TODO: Remove this
            /*if (true)
            {
                UsernameTextBox.Text = "admin";
                PasswordTextBox.Password = "admin";
                LoginButton_Click(null, null);
            }*/
        }

        private async void LoginButton_Click(object sender, RoutedEventArgs e)
        {
            var username = UsernameTextBox.Text;
            var password = PasswordTextBox.Password;

            try
            {
                await Utils.HClient.Request("/api/auth/login").PostJsonAndUnwrapAsync(new
                {
                    username,
                    password,
                });
            }
            catch (Exception ex)
            {
                Growl.Clear(Utils.MainWndToken);
                Utils.GrowlError(ex.Message);
                return;
            }

            // Login success
            Growl.Clear(Utils.MainWndToken);
            NavigationService.Navigate(new MainChatPage());
        }

        private void RegisterButton_Click(object sender, RoutedEventArgs e)
        {
            Growl.Clear(Utils.MainWndToken);
            NavigationService.Navigate(new RegisterPage());
        }
    }
}
