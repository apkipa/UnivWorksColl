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

namespace ChatWpf.Controls
{
    /// <summary>
    /// Interaction logic for RequestAddFriendDialog.xaml
    /// </summary>
    public partial class RequestAddFriendDialog : Border
    {
        public RequestAddFriendDialog()
        {
            InitializeComponent();
        }

        private async void SendButton_Click(object sender, RoutedEventArgs e)
        {
            var vm = (DataContext as ViewModel.RequestAddFriendDialogVM)!;
            await vm.AddFriendCmd.ExecuteAsync(null);
            HandyControl.Interactivity.ControlCommands.Close.Execute(null, (IInputElement)sender);
        }
    }
}
