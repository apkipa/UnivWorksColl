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
    /// Interaction logic for ManageGroupDialog.xaml
    /// </summary>
    public partial class ManageGroupDialog : Border
    {
        public ManageGroupDialog()
        {
            InitializeComponent();
        }

        private async void DeleteGroupButton_Click(object sender, RoutedEventArgs e)
        {
            var vm = (ManageGroupDialogVM)DataContext;
            bool doClose = true;
            try
            {
                await vm.DeleteGroupCmd.ExecuteAsync(null);
            }
            catch (OperationCanceledException)
            {
                doClose = false;
            }
            catch (Exception)
            {
                doClose = false;
                throw;
            }
            finally
            {
                if (doClose)
                {
                    HandyControl.Interactivity.ControlCommands.Close.Execute(null, (IInputElement)sender);
                }
            }
        }
    }
}
