using ChatWpf.Models;
using ChatWpf.ViewModel;
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
    /// Interaction logic for AddGroupMemberDialog.xaml
    /// </summary>
    public partial class AddGroupMemberDialog : Border
    {
        public AddGroupMemberDialog()
        {
            InitializeComponent();
        }

        private async void AddMemberButton_Click(object sender, RoutedEventArgs e)
        {
            var vm = (AddGroupMemberDialogVM)DataContext;
            var friend = ((FrameworkElement)sender).DataContext as AppUserInfo;
            await vm.AddMemberCmd.ExecuteAsync(friend);
            HandyControl.Interactivity.ControlCommands.Close.Execute(null, (IInputElement)sender);
        }
    }
}
