using ChatWpf.Misc;
using Flurl;
using Flurl.Http;
using HandyControl.Controls;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Security.Policy;
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
    /// Interaction logic for RegisterPage.xaml
    /// </summary>
    public partial class RegisterPage : Page
    {
        public RegisterPage()
        {
            InitializeComponent();
        }

        private void BackButton_Click(object sender, RoutedEventArgs e)
        {
            NavigationService.GoBack();
        }

        private async void RegisterButton_Click(object sender, RoutedEventArgs e)
        {
            var username = UsernameTextBox.Text;
            var password = PasswordTextBox.Password;

            await Utils.HClient.Request("/api/auth/register").PostJsonAndUnwrapAsync(new
            {
                username,
                password,
            });

            /*await Utils.HBase.AppendPathSegment("/api/auth/register").PostJsonAndUnwrapAsync(new
            {
                username,
                password,
            });*/

            Growl.Success("注册请求已经提交，请等待管理员审批。");
            NavigationService.GoBack();
        }
    }
}
